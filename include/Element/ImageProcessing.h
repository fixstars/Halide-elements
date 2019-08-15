#pragma once

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include <cassert>
#include <cmath>
#include <Halide.h>
#include "FixedPoint.h"
#include "Schedule.h"

namespace Halide {
namespace Element {
namespace {


Func affine(Func in, int32_t width, int32_t height, Param<float> degrees,
            Param<float> scale_x, Param<float> scale_y,
            Param<float> shift_x, Param<float> shift_y, Param<float> skew_y)
{
    // This affine transformation applies these operations below for an input image.
    //   1. scale about the origin
    //   2. shear in y direction
    //   3. rotation about the origin
    //   4. translation
    //
    // The applied matrix M consists of multiplied matrix by each operation:
    //
    //     M = M_translation * M_rotation * M_shear_y * M_scale
    //
    // So we can write a formula using Y, X, and M for affine transformation,
    //
    //     Y = M * X
    //
    //   where Y is the output image and X is the input image.
    //
    // But in Halide, we consider the formula from the output coordinates, so
    // calculate the inverse of matrix M, then rewrite as follows:
    //
    //     X = M^(-1) * Y
    //
    // Now let's take a look at the detail as below.
    // At first, matrix M is divided to translation and the others, and decompose it like this.
    //
    //         [ A  b ]
    //     Y = [      ] * X
    //         [ 0  1 ]
    //
    //   where A is M_rotation * M_shear_y * M_scale, and b = M_translation.
    //
    // Then, rewrite to the inverse formula.
    //
    //         [ A^(-1) -A^(-1)*b ]
    //     X = [                  ] * Y
    //         [   0         1    ]
    //
    // Next A is originally the matrix as follows:
    //
    //     A = M_rotation * M_shear_y * M_scale
    //
    //         [ cos_deg -sin_deg ]   [  1          0 ]   [ scale_x   0     ]
    //       = [                  ] * [               ] * [                 ]
    //         [ sin_deg  cos_deg ]   [ tan_skew_y  1 ]   [  0      scale_y ]
    //
    //         [ scale_x*(cos_deg-sin_deg*tan_skew_y)  -scale_y*sin_deg ]
    //       = [                                                        ]
    //         [ scale_x*(sin_deg+cos_deg*tan_skew_y)   scale_y*cos_deg ]
    //
    // Now we can get A^(-1) as follows:
    //
    //               1    [  scale_y*cos_deg                      scale_y*sin_deg                      ]
    //     A^(-1) = --- * [                                                                            ]
    //              det   [ -scale_x*(sin_deg+cos_deg*tan_skew_y) scale_x*(cos_deg-sin_deg*tan_skew_y) ]
    //
    //  where det = scale_x*scale_y.
    //
    Expr cos_deg = cos(degrees * (float)M_PI / 180.0f);
    Expr sin_deg = sin(degrees * (float)M_PI / 180.0f);
    Expr tan_skew_y = tan(skew_y * (float)M_PI / 180.0f);
    Expr det = scale_x * scale_y;
    Expr a00 = scale_y * cos_deg;
    Expr a10 = scale_y * sin_deg;
    Expr a20 = - (a00 * shift_x + a10 * shift_y);
    Expr a01 = - scale_x * (sin_deg + cos_deg * tan_skew_y);
    Expr a11 =   scale_x * (cos_deg - sin_deg * tan_skew_y);
    Expr a21 = - (a01 * shift_x + a11 * shift_y);

    // Here X can be described by Y, A, and b like this.
    //
    //         [ A^(-1) -A^(-1)*b ]
    //     X = [                  ] * Y
    //         [   0         1    ]
    //
    //   where Y = (x,y) and X = (tx, ty).
    //
    Var x, y;
    Func tx("tx"), ty("ty");
    tx(x, y) = cast<int>((a00*x + a10*y + a20) / det);
    ty(x, y) = cast<int>((a01*x + a11*y + a21) / det);

    // CAUTION: the coordinates of the input image cannot be out of the original width and height.
    //          so they are limited and the outside is set to 255 which is a white color.
    //
    Func affine("affine");
    Func limited = BoundaryConditions::constant_exterior(in, 255, 0, width, 0, height);
    affine(x, y) = limited(tx(x, y), ty(x, y));

    return affine;
}

template<typename T>
Func gaussian(Func in, int32_t width, int32_t height, int32_t window_width, int32_t window_height, Param<double> sigma)
{
    Var x{"x"}, y{"y"};

    Func clamped = BoundaryConditions::repeat_edge(in, 0, width, 0, height);
    RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
    Func kernel("kernel");
    kernel(x, y) = exp(-(x * x + y * y) / (2 * sigma * sigma));
    kernel.compute_root();

    Func kernel_sum("kernel_sum");
    kernel_sum(x) = sum(kernel(r.x, r.y));
    kernel_sum.compute_root();
    Func dst("dst");
    Expr dstval = cast<double>(sum(clamped(x + r.x, y + r.y) * kernel(r.x, r.y)));
    dst(x,y) = cast<T>(round(dstval / kernel_sum(0)));

    kernel.compute_root();
    kernel.bound(x, -(window_width / 2), window_width);
    kernel.bound(y, -(window_height / 2), window_height);
    schedule(kernel_sum, {1});

    return dst;
}

template<uint32_t NB, uint32_t FB>
Func convolution(Func in, int32_t width, int32_t height, Func kernel, int32_t kernel_size, int32_t unroll_factor) {
    Var x, y;

    Func bounded = BoundaryConditions::repeat_edge(in, 0, width, 0, height);

    Expr kh = Halide::div_round_to_zero(kernel_size, 2);
    RDom r(0, kernel_size, 0, kernel_size);

    Expr dx = r.x - kh;
    Expr dy = r.y - kh;

    Func k;
    k(x, y) = kernel(x, y);

    using FixedNB = FixedN<NB, FB>;
    FixedNB pv = to_fixed<NB, FB>(bounded(x+dx, y+dy));
    FixedNB kv{k(r.x, r.y)};

    Func out("out");
    out(x, y) = from_fixed<uint8_t>(sum_unroll(r, pv * kv));

    schedule(kernel, {5, 5});
    schedule(k, {5, 5});

    return out;
}


template<uint32_t frac_bits>
Func gamma_correction(Func in, Expr value)
{
    assert(value.type().is_float());

    Var x, y, c;

    Fixed<int16_t, frac_bits>  v = Fixed<int16_t, frac_bits>{in(c,x,y)};
    Func out;
    out(c,x,y) = static_cast<Expr>(to_fixed<int16_t, frac_bits>(fast_pow(from_fixed<float>(v), value)));
    return out;
}

Func optical_black_clamp(Func in, Expr clamp_value)
{
    Var x, y;
    Func out;
    out(x, y) = in(x, y) - min(in(x, y), clamp_value);
    return out;
}

namespace {
template<uint32_t frac_bits>
Fixed<int16_t, frac_bits> to_fixed16(Expr e)
{
    return to_fixed<int16_t, frac_bits>(e);
}
}

template<uint32_t frac_bits>
Func saturation_adjustment(Func in, Expr value)
{
    assert(value.type().is_float());

    Var x, y, c;

    Fixed<int16_t, frac_bits> zero = to_fixed16<frac_bits>(0);
    Fixed<int16_t, frac_bits> one = to_fixed16<frac_bits>(1);

    Fixed<int16_t, frac_bits> v = Fixed<int16_t, frac_bits>{in(x, y, c)};

    Func out;
    out(c, x, y) = static_cast<Expr>(select(c == 1, clamp(to_fixed16<frac_bits>(fast_pow(from_fixed<float>(v), value)), zero, one), v));
    return out;
}

template<uint32_t frac_bits>
Func denormalize(Func in)
{
    Var x, y, c;
    Fixed<int16_t, frac_bits> v = Fixed<int16_t, frac_bits>{in(c, x, y)};
    Func out;
    out(c, x, y) = cast<uint8_t>(clamp(from_fixed<float>(v) * 255.0f, 0.0f, 255.0f));
    return out;
}

template<uint32_t frac_bits>
Func color_interpolation_raw2rgb(Func in)
{
    Var x, y, c;

    Expr is_r  = (x % 2 == 0) && (y % 2 == 0);
    Expr is_gr = (x % 2 == 1) && (y % 2 == 0);
    Expr is_gb = (x % 2 == 0) && (y % 2 == 1);
    Expr is_b  = (x % 2 == 1) && (y % 2 == 1);

    Expr self = in(x, y);
    Expr hori = (in(x-1, y  ) + in(x+1, y  )) / 2;
    Expr vert = (in(x  , y-1) + in(x,   y+1)) / 2;
    Expr latt = (in(x-1, y  ) + in(x+1, y  ) + in(x,   y-1) + in(x,   y+1)) / 4;
    Expr diag = (in(x-1, y-1) + in(x+1, y-1) + in(x-1, y+1) + in(x+1, y+1)) / 4;

    // Assumes RAW has 10 bit resolutions
    Expr r = select(is_r, self, is_gr, hori, is_gb, vert, diag) >> 2;
    Expr g = select(is_r, latt, is_gr, diag, is_gb, diag, latt) >> 2;
    Expr b = select(is_r, diag, is_gr, vert, is_gb, hori, self) >> 2;

    Func out;
    out(c, x, y) = static_cast<Expr>(to_fixed16<frac_bits>(cast<float>(select(c == 0, r, c == 1, g, b) + 1) / 256.0f));
    return out;
}

template<uint32_t frac_bits>
Func color_interpolation_rgb2hsv(Func in)
{
    using Fixed16 = Fixed<int16_t, frac_bits>;

    Var x, y, c;
    Fixed16 zero = to_fixed16<frac_bits>(0);
    Fixed16 one  = to_fixed16<frac_bits>(1);
    Fixed16 two  = to_fixed16<frac_bits>(2);
    Fixed16 four = to_fixed16<frac_bits>(4);
    Fixed16 six  = to_fixed16<frac_bits>(6);

    Fixed16 r = Fixed16{in(0, x, y)};
    Fixed16 g = Fixed16{in(1, x, y)};
    Fixed16 b = Fixed16{in(2, x, y)};

    Fixed16 minv = min(r, min(g, b));
    Fixed16 maxv = max(r, max(g, b));
    Fixed16 diff = select(maxv == minv, Fixed16{1}, maxv - minv);

    Fixed16 h = select(maxv == minv, zero,
                        maxv == r,    (g-b)/diff,
                        maxv == g,    (b-r)/diff+two,
                                        (r-g)/diff+four);

    h = select(h < zero, h+six, h) / six;

    Fixed16 dmaxv = select(maxv == zero, Fixed16{1}, maxv);
    Fixed16 s = select(maxv == zero, zero, (maxv-minv)/dmaxv);
    Fixed16 v = maxv;

    Func out;
    out(c, x, y) = static_cast<Expr>(select(c == 0, h,
                                            c == 1, s,
                                                    v));
    return out;
}

template<uint32_t frac_bits>
Func color_interpolation_hsv2rgb(Func in)
{
    using Fixed16 = Fixed<int16_t, frac_bits>;

    Var x, y, c;

    Fixed16 zero = to_fixed16<frac_bits>(0);
    Fixed16 one  = to_fixed16<frac_bits>(1);
    Fixed16 six  = to_fixed16<frac_bits>(6);

    Fixed16 h = Fixed16{in(0, x, y)};
    Fixed16 s = Fixed16{in(1, x, y)};
    Fixed16 v = Fixed16{in(2, x, y)};

    Expr i = from_fixed<int32_t>(floor(six * h));

    Expr c0 = i == 0 || i == 6;
    Expr c1 = i == 1;
    Expr c2 = i == 2;
    Expr c3 = i == 3;
    Expr c4 = i == 4;
    Expr c5 = i == 5;

    Fixed16 f = six * h - floor(six * h);

    Fixed16 r = select(s > zero,
                        select(c0, v,
                                c1, v * (one - s * f),
                                c2, v * (one - s),
                                c3, v * (one - s),
                                c4, v * (one - s * (one - f)),
                                    v),
                        v);
    Fixed16 g = select(s > zero,
                        select(c0, v * (one - s * (one - f)),
                                c1, v,
                                c2, v,
                                c3, v * (one - s * f),
                                c4, v * (one - s),
                                    v * (one - s)),
                        v);

    Fixed16 b = select(s > zero,
                        select(c0, v * (one - s),
                                c1, v * (one - s),
                                c2, v * (one - s * (one - f)),
                                c3, v,
                                c4, v,
                                    v * (one - s * f)),
                        v);

    Func out;
    out(c, x, y) = static_cast<Expr>(select(c == 0, r, c == 1, g, b));
    return out;
}

Func merge3(Func in0, Func in1, Func in2, int32_t width, int32_t height) {
    Var x{"x"}, y{"y"}, c{"c"};
    Func merge3{"merge3"};

    merge3(c, x, y) = select(c == 0, in0(x, y),
                             c == 1, in1(x, y),
                             in2(x, y));
    merge3.unroll(c);

    return merge3;
}

Func merge4(Func in0, Func in1, Func in2, Func in3, int32_t width, int32_t height) {
    Var x{"x"}, y{"y"}, c{"c"};
    Func merge4{"merge4"};

    merge4(c, x, y) = select(c == 0, in0(x, y),
                             c == 1, in1(x, y),
                             c == 2, in2(x, y),
                             in3(x, y));
    merge4.unroll(c);

    return merge4;
}

Func bitonic_sort(Func input, int32_t size, int32_t width, int32_t height) {
    // rounding the size of input up to power of two
    input = BoundaryConditions::constant_exterior(input, input.value().type().max(),
        {{0, size}, {0, cast<int>(width)}, {0, cast<int>(height)}});
    size = std::pow(2, std::ceil(std::log2((double)size)));

    Func next, prev = input;

    Var x{"x"}, y{"y"}, i{"i"};

    for (int pass_size = 1; pass_size < size; pass_size <<= 1) {
        for (int chunk_size = pass_size; chunk_size > 0; chunk_size >>= 1) {
            next = Func("bitonic_pass_" + std::to_string(pass_size) + "_" + std::to_string(chunk_size));
            Expr chunk_start = (i/(2*chunk_size))*(2*chunk_size);
            Expr chunk_end = (i/(2*chunk_size) + 1)*(2*chunk_size);
            Expr chunk_middle = chunk_start + chunk_size;
            Expr chunk_index = i - chunk_start;
            if (pass_size == chunk_size && pass_size > 1) {
                // Flipped pass
                Expr partner = 2*chunk_middle - i - 1;
                // We need a clamp here to help out bounds inference
                partner = clamp(partner, chunk_start, chunk_end-1);
                next(i, x, y) = select(i < chunk_middle,
                                       min(prev(i, x, y), prev(partner, x, y)),
                                       max(prev(i, x, y), prev(partner, x, y)));
            } else {
                // Regular pass
                Expr partner = chunk_start + (chunk_index + chunk_size) % (chunk_size*2);
                next(i, x, y) = select(i < chunk_middle,
                                       min(prev(i, x, y), prev(partner, x, y)),
                                       max(prev(i, x, y), prev(partner, x, y)));


            }

            prev.compute_at(next, x);
            next.unroll(i);
            prev = next;
        }
    }

    return next;
}

Func median(Func in, int32_t width, int32_t height, int32_t window_width, int32_t window_height) {
    Expr offset_x = window_width / 2, offset_y = window_height / 2;
    int32_t window_size = window_width * window_height;

    Func clamped = BoundaryConditions::repeat_edge(in, {{0, width}, {0, height}});
    RDom r(-offset_x, window_width, -offset_y, window_height);

    Func window("window");

    Var x{"x"}, y{"y"}, i{"i"};
    window(i, x, y) = undef(in.value().type());
    window((r.x + offset_x) + (r.y + offset_y) * window_width, x, y) =
        clamped(x + r.x, y + r.y);
    window.unroll(i);
    window.update(0).unroll(r.x);
    window.update(0).unroll(r.y);

    Func sorted = bitonic_sort(window, window_size, width, height);
    Func median("median");
    median(x, y) = sorted(window_size / 2, x, y);

    sorted.compute_at(median, x);
    schedule(window, {window_size, width, height});
    return median;
}

template<typename T>
Func laplacian(Func in, int32_t width, int32_t height) {
    Var x{"x"}, y{"y"};

    Func clamped = BoundaryConditions::repeat_edge(in, {{0, width}, {0, height}});
    Func kernel("kernel");
    kernel(x, y) = cast<double>(-1);
    kernel(0, 0) = cast<double>(8);

    RDom r(-1, 3, -1, 3);
    Func dst("dst");
    Expr dstval = sum(cast<double>(clamped(x + r.x, y + r.y)) * kernel(r.x, r.y));
    dstval = select(dstval < 0, -dstval, dstval);
    dstval = select(dstval > type_of<T>().max(), cast<double>(type_of<T>().max()), dstval);
    dst(x, y) = cast<T>(dstval);

    kernel.compute_root();
    kernel.bound(x, -1, 3);
    kernel.bound(y, -1, 3);

    return dst;
}

template<typename T>
Func prewitt(Func input, int32_t width, int32_t height)
{
    Var x, y;
    Func input_f("input_f");
    input_f(x, y) = cast<float>(input(x, y));

    Func clamped = BoundaryConditions::repeat_edge(input_f, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});

    Func diff_x("diff_x"), diff_y("diff_y");
    diff_x(x, y) = -clamped(x-1, y-1) + clamped(x+1, y-1) +
                   -clamped(x-1, y  ) + clamped(x+1, y  ) +
                   -clamped(x-1, y+1) + clamped(x+1, y+1);

    diff_y(x, y) = -clamped(x-1, y-1) + clamped(x-1, y+1) +
                   -clamped(x  , y-1) + clamped(x  , y+1) +
                   -clamped(x+1, y-1) + clamped(x+1, y+1);

    Func output("output");
    output(x, y) = cast<T>(hypot(diff_x(x, y), diff_y(x, y)));

    return output;
}

template<typename T>
Func sobel(Func input, int32_t width, int32_t height)
{
    Var x, y;
    Func input_f("input_f");
    input_f(x, y) = cast<float>(input(x, y));

    Func clamped = BoundaryConditions::repeat_edge(input_f, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});

    Func diff_x("diff_x"), diff_y("diff_y");
    diff_x(x, y) = -clamped(x-1, y-1) + clamped(x+1, y-1) +
                   -2 * clamped(x-1, y  ) + 2 * clamped(x+1, y  ) +
                   -clamped(x-1, y+1) + clamped(x+1, y+1);

    diff_y(x, y) = -clamped(x-1, y-1) + clamped(x-1, y+1) +
                   -2 * clamped(x  , y-1) + 2 * clamped(x  , y+1) +
                   -clamped(x+1, y-1) + clamped(x+1, y+1);

    Func output("output");
    output(x, y) = cast<T>(hypot(diff_x(x, y), diff_y(x, y)));

    schedule(diff_x, {width, height});
    schedule(diff_y, {width, height});

    return output;
}

template <typename T>
Func tm_ssd(Func src0, Func src1, const int32_t img_width, const int32_t img_height, const int32_t tmp_width, const int32_t tmp_height)
{
    Var x{"x"}, y{"y"};

    RDom r(0, tmp_width, 0, tmp_height);

    Func out{"out"};
    Expr diff{"diff"};
    diff = cast<double>(src0(x + r.x, y + r.y)) - cast<double>(src1(r.x, r.y));
    out(x, y) = sum(diff * diff);

    return out;
}

template <typename T>
Func tm_sad(Func src0, Func src1, const int32_t img_width, const int32_t img_height, const int32_t tmp_width, const int32_t tmp_height)
{
    Var x{"x"}, y{"y"};

    RDom r(0, tmp_width, 0, tmp_height);

    Func out{"out"};
    Expr diff{"diff"};
    diff = cast<double>(src0(x + r.x, y + r.y)) - cast<double>(src1(r.x, r.y));
    out(x, y) = sum(abs(diff));

    return out;
}

template <typename T>
Func tm_ncc(Func src0, Func src1, const int32_t img_width, const int32_t img_height, const int32_t tmp_width, const int32_t tmp_height)
{
    Var x{"x"}, y{"y"};

    RDom r(0, tmp_width, 0, tmp_height);

    Func out{"out"};
    Expr sum1{"sum1"}, sum2{"sum2"}, sum3{"sum3"};
    sum1 = sum(cast<double>(src0(x + r.x, y + r.y)) * cast<double>(src1(r.x, r.y)));
    sum2 = sum(cast<double>(src0(x + r.x, y + r.y)) * cast<double>(src0(x + r.x, y + r.y)));
    sum3 = sum(cast<double>(src1(r.x, r.y)) * cast<double>(src1(r.x, r.y)));
    out(x, y) = sum1 / sqrt(sum2 * sum3);

    return out;
}

template <typename T>
Func tm_zncc(Func src0, Func src1, const int32_t img_width, const int32_t img_height, const int32_t tmp_width, const int32_t tmp_height)
{
    Var x{"x"}, y{"y"};

    RDom r(0, tmp_width, 0, tmp_height);

    Func out{"out"};
    Expr tmp_size = cast<double>(tmp_width) * cast<double>(tmp_height);
    Expr avr0{"avr0"}, avr1{"avr1"};
    avr0 = sum(cast<double>(src0(x + r.x, y + r.y))) / tmp_size;
    avr1 = sum(cast<double>(src1(r.x, r.y))) / tmp_size;

    Expr sum1{"sum1"}, sum2{"sum2"}, sum3{"sum3"};
    sum1 = sum(cast<double>(src0(x + r.x, y + r.y) - avr0) * cast<double>(src1(r.x, r.y) - avr1));
    sum2 = sum(cast<double>(src0(x + r.x, y + r.y) - avr0) * cast<double>(src0(x + r.x, y + r.y) - avr0));
    sum3 = sum(cast<double>(src1(r.x, r.y) - avr1) * cast<double>(src1(r.x, r.y) - avr1));
    out(x, y) = sum1 / sqrt(sum2 * sum3);

    return out;
}

Func set_scalar(Expr val)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = val;

    return dst;
}

Func split3(Func src, int32_t widthe, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    dst(x, y) = Tuple(src(0, x, y), src(1, x, y), src(2, x, y));

    return dst;
}

Func split4(Func src, int32_t widthe, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    dst(x, y) = Tuple(src(0, x, y), src(1, x, y), src(2, x, y), src(3, x, y));

    return dst;
}

template<typename T>
Func sad(Func input0, Func input1, int32_t width, int32_t height)
{
	Var x{"x"}, y{"y"};
	Expr srcval0 = cast<int64_t>(input0(x,y));
	Expr srcval1 = cast<int64_t>(input1(x,y));
	Expr diffval = srcval0 - srcval1;

	Func output{"output"};
	output(x,y) = select(diffval<0, cast<T>(-diffval), cast<T>(diffval));

	return output;
}

template<typename T>Func bilateral(Func src, int32_t width, int32_t height, Expr wSize, Expr color, Expr space)
{
    return Func();
}
//for uint8_t and for uint16_t

template<> Func bilateral<uint8_t>(Func src, int32_t width, int32_t height, Expr wSize, Expr color, Expr space){
    Func dst{"dst"};
    Var x{"x"}, y{"y"};

    Func kernel_r{"kernel_r"};
    Var i{"i"};
    kernel_r(i) = exp(cast<double>(-0.5f) * cast<double>(i) * cast<double>(i)/ (color * color));
    schedule(kernel_r, {256});

    Expr wRadius = cast<int>(wSize/2);
    RDom w{0, wSize, 0, wSize, "w"};

    Expr diff_x = cast<double>(x-wRadius);
    Expr diff_y = cast<double>(y-wRadius);
    Expr r = sqrt(diff_y*diff_y + diff_x*diff_x);
    Func kernel_d;
    kernel_d(x, y) = select( r > wRadius, 0,
                             exp(-0.5f * (diff_x * diff_x + diff_y * diff_y) / (space * space)));
    schedule(kernel_d, {5, 5});

    Func clamped = BoundaryConditions::repeat_edge(src, 0, width, 0, height);
    Func bri;
    bri(x, y) = clamped(x-wRadius, y-wRadius);

    Func num;
    num(x, y) = sum_unroll(w, kernel_d(w.x, w.y)
                    * select(src(x, y) > bri(x+w.x, y+w.y),
                             kernel_r(src(x, y)-bri(x+w.x, y+w.y)),
                             kernel_r(bri(x+w.x, y+w.y)-src(x, y)))
                    * bri(w.x+x, w.y+y))
                /sum_unroll(w, kernel_d(w.x, w.y)
                     * select(src(x, y) > bri(x+w.x, y+w.y),
                              kernel_r(src(x, y)-bri(x+w.x, y+w.y)),
                              kernel_r(bri(x+w.x, y+w.y)-src(x, y))));

    dst(x, y) = select(
        cast<float>(num(x, y)-floor(num(x, y))-0.5f) > (std::numeric_limits<float>::epsilon)()
        || (cast<uint8_t>(num(x, y)))%2==1,
        cast<uint8_t>(num(x, y) + 0.5f),
        cast<uint8_t>(num(x, y)));

    schedule(num, {width, height});

    return dst;
}

template<> Func bilateral<uint16_t>(Func src, int32_t width, int32_t height, Expr wSize, Expr color, Expr space){
    Func dst{"dst"};
    Var x{"x"}, y{"y"};

    Expr wRadius = cast<int>(wSize/2);
    RDom w{0, wSize, 0, wSize, "w"};

    Expr diff_x = cast<double>(x-wRadius);
    Expr diff_y = cast<double>(y-wRadius);
    Expr r = sqrt(diff_y*diff_y + diff_x*diff_x);
    Func kernel_d;
    kernel_d(x, y) = select( r > wRadius, 0,
                             exp(-0.5f * (diff_x * diff_x + diff_y * diff_y) / (space * space)));
    schedule(kernel_d, {5, 5});

    Func clamped = BoundaryConditions::repeat_edge(src, 0, width, 0, height);
    Func bri;
    bri(x, y) = clamped(x-wRadius, y-wRadius);

    Func num;
    num(x, y) = sum_unroll(w, kernel_d(w.x, w.y)
                    * exp(-0.5f * (cast<double>(src(x, y))-cast<double>(bri(x+w.x, y+w.y)))
                                * (cast<double>(src(x, y))-cast<double>(bri(x+w.x, y+w.y)))
                                / (color*color))
                    * bri(w.x+x, w.y+y))
                /sum_unroll(w, kernel_d(w.x, w.y)
                     * exp(-0.5f * (cast<double>(src(x, y))-cast<double>(bri(x+w.x, y+w.y)))
                                 * (cast<double>(src(x, y))-cast<double>(bri(x+w.x, y+w.y)))
                                 / (color*color)));

    dst(x, y) = select(
        cast<float>(num(x, y)-floor(num(x, y))-0.5f) > (std::numeric_limits<float>::epsilon)()
        || (cast<uint16_t>(num(x, y)))%2==1,
        cast<uint16_t>(num(x, y) + 0.5f),
        cast<uint16_t>(num(x, y)));

    schedule(num, {width, height});

    return dst;
}

template <typename T>
Func scale_NN(Func src, int32_t in_width, int32_t in_height, int32_t out_width, int32_t out_height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    Expr srcx = cast<int>(cast<float>(cast<float>(x)+cast<float>(0.5f))*cast<float>(in_width)/cast<float>(out_width));
    Expr srcy = cast<int>(cast<float>(cast<float>(y)+cast<float>(0.5f))*cast<float>(in_height)/cast<float>(out_height));
    Func clamped = BoundaryConditions::repeat_edge(src, 0, in_width, 0, in_height);

    dst(x, y) = clamped(srcx, srcy);
    return dst;
}

template <typename T>
Func scale_bicubic(Func src, int32_t in_width, int32_t in_height, int32_t out_width, int32_t out_height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};

    ////////////////////////////////////////////////////////////////////////////
    Expr srcx = cast<float>(cast<float>(x)+cast<float>(0.5f))*cast<float>(in_width)/cast<float>(out_width);
    Expr srcy = cast<float>(cast<float>(y)+cast<float>(0.5f))*cast<float>(in_height)/cast<float>(out_height);

    // At this point, Simplify changes the float arithmetic order
    // They seems expand the equation even though the variables are float
    // (if the flaot numbers are both constant, they do not expand. Check the following test result)
    //
    // This might make the error and the largest difference between actual and expected is 9 where 1024x768->500x500
    //
    //  test result
        // (x + y) * a  ->  (x + y) * a
        // (x + y) / a -> (x + y) * (1/a)
        // (x * a) + (y * a) -> (x + y) * a
        // (x + y) * b / c -> (x * b / c) + (x * b / c)
        // (x + a) * b -> (x * b) + (a * b)
        // (x + a) / b -> (x / b) + (a / b)
        // (a + b) / c -> (a + b) * c
    //
    // Guessing that srcx and srcy are passing:
    // ( X + a ) * b / c -> ( X + a ) * b * 1/c
    //                   -> ( X + a) * (b * 1/c)
    //                   -> (X * (b * 1/c)) + (a * (b * 1/c))
    //
    // Check Simplify.cpp
        // void visit(const Mul *op)
        // void visit(const Div *op)
    ////////////////////////////////////////////////////////////////////////////
    srcx = srcx - 0.5f;
    srcy = srcy - 0.5f;

    Expr diffx = srcx - cast<float>(floor(srcx));
    Expr diffy = srcy - cast<float>(floor(srcy));

    RDom r{-1, 4, -1, 4, "r"};
    Func weight, value, totalWeight;
    Expr alpha = -1.0f;

    Expr iX = select(r.x > diffx, cast<float>( r.x - diffx), cast<float>(diffx - r.x) );
    Expr iY = select(r.y > diffy, cast<float>(r.y - diffy), cast<float>(diffy - r.y));
    Expr iX2 = iX * iX;
    Expr iX3 = iX * iX * iX;
    Expr iY2 = iY * iY;
    Expr iY3 = iY * iY * iY;
    iX = select(iX <= 1.0f, (alpha+2)*iX3-(alpha+3)*iX2+1,
                    iX < 2.0f, alpha*iX3-5.0f*alpha*iX2+8.0f*alpha*iX-4.0f*alpha,
                    0.0f);
    iY = select(iY <= 1, (alpha+2)*iY3-(alpha+3)*iY2+1,
                    iY < 2.0f, alpha*iY3-5.0f*alpha*iY2+8.0f*alpha*iY-4.0f*alpha,
                    0.0f);

    Expr tmp = cast<float>(iX * iY);
    Func clamped = BoundaryConditions::repeat_edge(src, 0, in_width, 0, in_height);
    value(x, y) = sum(cast<double>(tmp*clamped(cast<int>(srcx + r.x), cast<int>(srcy + r.y))));

    totalWeight(x, y) = sum(tmp);
    totalWeight(x, y) = select(totalWeight(x,y) < 0, -totalWeight(x, y), totalWeight(x,y));

    value(x,y) = select(totalWeight(x,y) ==cast<double>(0.0f), cast<double>(0.5f),
                        value(x,y)/cast<double>(totalWeight(x, y)) +cast<double>(0.5f));
    dst(x, y) = cast<T>(clamp(value(x,y),
                              cast<double>(type_of<T>().min()),
                              cast<double>(type_of<T>().max())));

    schedule(value, {out_width, out_height});
    schedule(totalWeight, {out_width, out_height});
    schedule(dst, {out_width, out_height});
    return dst;
}

template<typename T>
Func warp_map_NN(Func src0, Func src1, Func src2, int32_t border_type, Expr border_value, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};

    Expr srcx = src1(x, y);
    Expr srcy = src2(x, y);

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = cast<int>(floor(srcy));
    Expr j = cast<int>(floor(srcx));
    Func type0 = BoundaryConditions::constant_exterior(src0, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src0, 0, width, 0, height);
    dst(x, y) = select(border_type==1, type1(j, i), type0(j, i));

    return dst;
}

template<typename T>
Func warp_map_bilinear(Func src0, Func src1, Func src2, int32_t border_type, Expr border_value, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};

    Expr srcx = src1(x, y);
    Expr srcy = src2(x, y);

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = srcy - 0.5f;
    Expr j = srcx - 0.5f;
    Expr xf = cast<int>(j);
    Expr yf = cast<int>(i);
    xf = xf - (xf > j);
    yf = yf - (yf > i);

    Func type0 = BoundaryConditions::constant_exterior(src0, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src0, 0, width, 0, height);
    Expr d[4];
    d[0] = select(border_type==1, type1(xf, yf), type0(xf, yf));
    d[1] = select(border_type==1, type1(xf+1, yf), type0(xf+1, yf));
    d[2] = select(border_type==1, type1(xf, yf+1), type0(xf, yf+1));
    d[3] = select(border_type==1, type1(xf+1, yf+1), type0(xf+1, yf+1));

    Expr dx = min(max(0.0f, j-cast<float>(xf)), 1.0f);
    Expr dy = min(max(0.0f, i-cast<float>(yf)), 1.0f);
    Expr value = (d[0]*(1.0f-dx)*(1.0f-dy) + d[1]*dx*(1.0f-dy))
                 + (d[2]*(1.0f-dx)*dy + d[3]*dx*dy);
    dst(x, y) = cast<T>(value+0.5f);

    return dst;
}

template<typename T>
Func warp_map_bicubic(Func src0, Func src1, Func src2, int32_t border_type, Expr border_value, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};

    Expr srcx = src1(x, y);
    Expr srcy = src2(x, y);

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = srcy - 0.5f;
    Expr j = srcx - 0.5f;
    Expr xf = cast<int>(j-1.0f);
    Expr yf = cast<int>(i-1.0f);
    xf = xf - (xf > j-1.0f);
    yf = yf - (yf > i-1.0f);

    Func type0 = BoundaryConditions::constant_exterior(src0, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src0, 0, width, 0, height);

    RDom r{0, 4, 0, 4, "r"};
    Expr d = cast<float>(select(border_type==1,type1(xf+r.x, yf+r.y) ,type0(xf+r.x, yf+r.y)));

    Expr dx = min(max(0.0f, j-cast<float>(xf)-1.0f), 1.0f);
    Expr dy = min(max(0.0f, i-cast<float>(yf)-1.0f), 1.0f);

    static const float a = -0.75f;
    Expr w0 = ((a*(dx+1.0f)-5.0f*a)*(dx+1.0f)+8.0f*a)*(dx+1.0f)-4.0f*a;
    Expr w1 = ((a+2.0f)*dx-(a+3.0f))*dx*dx+1.0f;
    Expr w2 = ((a+2.0f)*(1.0f-dx)-(a+3.0f))*(1.0f-dx)*(1.0f-dx)+1.0f;
    Expr w3 = 1.0f - w2 - w1 -w0;

    d = select(r.x == 0, d*w0,
               r.x == 1, d*w1,
               r.x == 2, d*w2,
               r.x == 3, d*w3, d);

    w0 = ((a*(dy+1.0f)-5.0f*a)*(dy+1.0f)+8.0f*a)*(dy+1.0f)-4.0f*a;
    w1 = ((a+2.0f)*dy-(a+3.0f))*dy*dy+1.0f;
    w2 = ((a+2.0f)*(1.0f-dy)-(a+3.0f))*(1.0f-dy)*(1.0f-dy)+1.0f;
    w3 = 1.0f - w2 - w1 -w0;

    Expr c0 = sum(select(r.y ==0, d, 0))*w0;
    Expr c1 = sum(select(r.y ==1, d, 0))*w1;
    Expr c2 = sum(select(r.y ==2, d, 0))*w2;
    Expr c3 = sum(select(r.y ==3, d, 0))*w3;

    Expr value = c0 + c1 + c2 + c3;
    value = select(value > cast<float>(type_of<T>().max()), cast<float>(type_of<T>().max()),
                   value < cast<float>(type_of<T>().min()), cast<float>(type_of<T>().min()),
                   value + 0.5f);
    dst(x, y) = cast<T>(value);

    return dst;
}

template<typename T>
Func warp_affine_NN(Func src, int32_t border_type, Expr border_value, Func transform, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    Expr orgx = cast<float>(x) + 0.5f;
    Expr orgy = cast<float>(y) + 0.5f;
    Expr srcx = cast<float>(transform(2)) + cast<float>(transform(1)) * orgy;
    Expr srcy = cast<float>(transform(5)) + cast<float>(transform(4)) * orgy;
    srcx = srcx + cast<float>(transform(0)) * orgx;
    srcy = srcy + cast<float>(transform(3)) * orgx;

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = cast<int>(floor(srcy));
    Expr j = cast<int>(floor(srcx));

    Func type0 = BoundaryConditions::constant_exterior(src, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src, 0, width, 0, height);
    dst(x, y) = select(border_type==1, type1(j, i), type0(j, i));

    return dst;
}

template<typename T>
Func warp_affine_bilinear(Func src, int32_t border_type, Expr border_value, Func transform, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    Expr orgx = cast<float>(x) + 0.5f;
    Expr orgy = cast<float>(y) + 0.5f;
    Expr srcx = cast<float>(transform(2)) + cast<float>(transform(1)) * orgy;
    Expr srcy = cast<float>(transform(5)) + cast<float>(transform(4)) * orgy;
    srcx = srcx + cast<float>(transform(0)) * orgx;
    srcy = srcy + cast<float>(transform(3)) * orgx;

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = srcy - 0.5f;
    Expr j = srcx - 0.5f;
    Expr xf = cast<int>(j);
    Expr yf = cast<int>(i);
    xf = xf - (xf > j);
    yf = yf - (yf > i);

    Func type0 = BoundaryConditions::constant_exterior(src, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src, 0, width, 0, height);
    Expr d[4];
    d[0] = select(border_type==1, type1(xf, yf), type0(xf, yf));
    d[1] = select(border_type==1, type1(xf+1, yf), type0(xf+1, yf));
    d[2] = select(border_type==1, type1(xf, yf+1), type0(xf, yf+1));
    d[3] = select(border_type==1, type1(xf+1, yf+1), type0(xf+1, yf+1));

    Expr dx = min(max(0.0f, j-cast<float>(xf)), 1.0f);
    Expr dy = min(max(0.0f, i-cast<float>(yf)), 1.0f);
    Expr value = (d[0]*(1.0f-dx)*(1.0f-dy) + d[1]*dx*(1.0f-dy))
                 + (d[2]*(1.0f-dx)*dy + d[3]*dx*dy);
    dst(x, y) = cast<T>(value+0.5f);
    return dst;
}

template<typename T>
Func warp_affine_bicubic(Func src, int32_t border_type, Expr border_value, Func transform, int32_t width, int32_t height)
{
    Var x{"x"};
    Var y{"y"};
    Func dst{"dst"};

    Expr orgx = cast<float>(x) + 0.5f;
    Expr orgy = cast<float>(y) + 0.5f;
    Expr srcx = cast<float>(transform(2)) + cast<float>(transform(1)) * orgy;
    Expr srcy = cast<float>(transform(5)) + cast<float>(transform(4)) * orgy;
    srcx = srcx + cast<float>(transform(0)) * orgx;
    srcy = srcy + cast<float>(transform(3)) * orgx;

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = srcy - 0.5f;
    Expr j = srcx - 0.5f;
    Expr xf = cast<int>(j-1.0f);
    Expr yf = cast<int>(i-1.0f);
    xf = xf - (xf > j-1.0f);
    yf = yf - (yf > i-1.0f);

    Func type0 = BoundaryConditions::constant_exterior(src, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src, 0, width, 0, height);

    RDom r{0, 4, 0, 4, "r"};
    Expr d = cast<float>(select(border_type==1,type1(xf+r.x, yf+r.y) ,type0(xf+r.x, yf+r.y)));

    Expr dx = min(max(0.0f, j-cast<float>(xf)-1.0f), 1.0f);
    Expr dy = min(max(0.0f, i-cast<float>(yf)-1.0f), 1.0f);

    static const float a = -0.75f;
    Expr w0 = ((a*(dx+1.0f)-5.0f*a)*(dx+1.0f)+8.0f*a)*(dx+1.0f)-4.0f*a;
    Expr w1 = ((a+2.0f)*dx-(a+3.0f))*dx*dx+1.0f;
    Expr w2 = ((a+2.0f)*(1.0f-dx)-(a+3.0f))*(1.0f-dx)*(1.0f-dx)+1.0f;
    Expr w3 = 1.0f - w2 - w1 -w0;

    d = select(r.x == 0, d*w0,
               r.x == 1, d*w1,
               r.x == 2, d*w2,
               r.x == 3, d*w3, d);

    w0 = ((a*(dy+1.0f)-5.0f*a)*(dy+1.0f)+8.0f*a)*(dy+1.0f)-4.0f*a;
    w1 = ((a+2.0f)*dy-(a+3.0f))*dy*dy+1.0f;
    w2 = ((a+2.0f)*(1.0f-dy)-(a+3.0f))*(1.0f-dy)*(1.0f-dy)+1.0f;
    w3 = 1.0f - w2 - w1 -w0;

    Expr c0 = sum(select(r.y ==0, d, 0))*w0;
    Expr c1 = sum(select(r.y ==1, d, 0))*w1;
    Expr c2 = sum(select(r.y ==2, d, 0))*w2;
    Expr c3 = sum(select(r.y ==3, d, 0))*w3;

    Expr value = c0 + c1 + c2 + c3;
    value = select(value > cast<float>(type_of<T>().max()), cast<float>(type_of<T>().max()),
                   value < cast<float>(type_of<T>().min()), cast<float>(type_of<T>().min()),
                   value + 0.5f);
    dst(x, y) = cast<T>(value);
    return dst;

    schedule(dst, {width, height});
}

template <typename T>
Func threshold_min(Func src, Expr threshold)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = select (src(x, y) > threshold, threshold, src(x,y));

    return dst;
}

template <typename T>
Func threshold_max(Func src, Expr threshold)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = select (src(x, y) > threshold, src(x,y), threshold);

    return dst;
}

template <typename T>
Func threshold_binary(Func src, Expr threshold, Expr value)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = select (src(x, y) > threshold, value, 0);

    return dst;
}

template <typename T>
Func threshold_binary_inv(Func src, Expr threshold, Expr value)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = select (src(x, y) > threshold, 0, value);

    return dst;
}

template <typename T>
Func threshold_tozero(Func src, Expr threshold)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = select (src(x, y) > threshold, src(x,y), 0);

    return dst;
}

template <typename T>
Func threshold_tozero_inv(Func src, Expr threshold)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = select (src(x, y) > threshold, 0, src(x,y));

    return dst;
}

template<typename T>
Func warp_perspective_NN(Func src, int32_t border_type, Expr border_value, Func transform, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    Expr orgx = cast<float>(x) + 0.5f;
    Expr orgy = cast<float>(y) + 0.5f;
    Expr inv_w = cast<float>(transform(8)) + cast<float>(transform(7)) * orgy;
    inv_w = 1.0f / (inv_w + cast<float>(transform(6)) * orgx);
    Expr srcx = cast<float>(transform(2)) + cast<float>(transform(1)) * orgy;
    Expr srcy = cast<float>(transform(5)) + cast<float>(transform(4)) * orgy;
    srcx = (srcx + cast<float>(transform(0)) * orgx) * inv_w;
    srcy = (srcy + cast<float>(transform(3)) * orgx) * inv_w;

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = cast<int>(floor(srcy));
    Expr j = cast<int>(floor(srcx));

    Func type0 = BoundaryConditions::constant_exterior(src, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src, 0, width, 0, height);
    dst(x, y) = select(border_type==1, type1(j, i), type0(j, i));

    return dst;
}

template<typename T>
Func warp_perspective_bilinear(Func src, int32_t border_type, Expr border_value, Func transform, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    Expr orgx = cast<float>(x) + 0.5f;
    Expr orgy = cast<float>(y) + 0.5f;
    Expr inv_w = cast<float>(transform(8)) + cast<float>(transform(7)) * orgy;
    inv_w = 1.0f / (inv_w + cast<float>(transform(6)) * orgx);
    Expr srcx = cast<float>(transform(2)) + cast<float>(transform(1)) * orgy;
    Expr srcy = cast<float>(transform(5)) + cast<float>(transform(4)) * orgy;
    srcx = (srcx + cast<float>(transform(0)) * orgx) * inv_w;
    srcy = (srcy + cast<float>(transform(3)) * orgx) * inv_w;

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = srcy - 0.5f;
    Expr j = srcx - 0.5f;
    Expr xf = cast<int>(j);
    Expr yf = cast<int>(i);
    xf = xf - (xf > j);
    yf = yf - (yf > i);

    Func type0 = BoundaryConditions::constant_exterior(src, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src, 0, width, 0, height);
    Expr d[4];
    d[0] = select(border_type==1, type1(xf, yf), type0(xf, yf));
    d[1] = select(border_type==1, type1(xf+1, yf), type0(xf+1, yf));
    d[2] = select(border_type==1, type1(xf, yf+1), type0(xf, yf+1));
    d[3] = select(border_type==1, type1(xf+1, yf+1), type0(xf+1, yf+1));

    Expr dx = min(max(0.0f, j-cast<float>(xf)), 1.0f);
    Expr dy = min(max(0.0f, i-cast<float>(yf)), 1.0f);
    Expr value = (d[0]*(1.0f-dx)*(1.0f-dy) + d[1]*dx*(1.0f-dy))
                 + (d[2]*(1.0f-dx)*dy + d[3]*dx*dy);
    dst(x, y) = cast<T>(value+0.5f);
    return dst;
}

template<typename T>
Func warp_perspective_bicubic(Func src, int32_t border_type, Expr border_value, Func transform, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    Expr orgx = cast<float>(x) + 0.5f;
    Expr orgy = cast<float>(y) + 0.5f;
    Expr inv_w = cast<float>(transform(8)) + cast<float>(transform(7)) * orgy;
    inv_w = 1.0f / (inv_w + cast<float>(transform(6)) * orgx);
    Expr srcx = cast<float>(transform(2)) + cast<float>(transform(1)) * orgy;
    Expr srcy = cast<float>(transform(5)) + cast<float>(transform(4)) * orgy;
    srcx = (srcx + cast<float>(transform(0)) * orgx) * inv_w;
    srcy = (srcy + cast<float>(transform(3)) * orgx) * inv_w;

    /* avoid overflow from X-1 to X+2 */
    Expr imin = cast<float>(type_of<int>().min() + 1);
    Expr imax = cast<float>(type_of<int>().max() - 2);
    srcx = select(srcx<imin, imin, select(srcx>imax, imax, srcx));
    srcy = select(srcy<imin, imin, select(srcy>imax, imax, srcy));

    Expr i = srcy - 0.5f;
    Expr j = srcx - 0.5f;
    Expr xf = cast<int>(j-1.0f);
    Expr yf = cast<int>(i-1.0f);
    xf = xf - (xf > j-1.0f);
    yf = yf - (yf > i-1.0f);

    Func type0 = BoundaryConditions::constant_exterior(src, border_value, 0, width, 0, height);
    Func type1 = BoundaryConditions::repeat_edge(src, 0, width, 0, height);

    RDom r{0, 4, 0, 4, "r"};
    Expr d = cast<float>(select(border_type==1,type1(xf+r.x, yf+r.y) ,type0(xf+r.x, yf+r.y)));

    Expr dx = min(max(0.0f, j-cast<float>(xf)-1.0f), 1.0f);
    Expr dy = min(max(0.0f, i-cast<float>(yf)-1.0f), 1.0f);

    static const float a = -0.75f;
    Expr w0 = ((a*(dx+1.0f)-5.0f*a)*(dx+1.0f)+8.0f*a)*(dx+1.0f)-4.0f*a;
    Expr w1 = ((a+2.0f)*dx-(a+3.0f))*dx*dx+1.0f;
    Expr w2 = ((a+2.0f)*(1.0f-dx)-(a+3.0f))*(1.0f-dx)*(1.0f-dx)+1.0f;
    Expr w3 = 1.0f - w2 - w1 -w0;

    d = select(r.x == 0, d*w0,
               r.x == 1, d*w1,
               r.x == 2, d*w2,
               r.x == 3, d*w3, d);

    w0 = ((a*(dy+1.0f)-5.0f*a)*(dy+1.0f)+8.0f*a)*(dy+1.0f)-4.0f*a;
    w1 = ((a+2.0f)*dy-(a+3.0f))*dy*dy+1.0f;
    w2 = ((a+2.0f)*(1.0f-dy)-(a+3.0f))*(1.0f-dy)*(1.0f-dy)+1.0f;
    w3 = 1.0f - w2 - w1 -w0;

    Expr c0 = sum(select(r.y ==0, d, 0))*w0;
    Expr c1 = sum(select(r.y ==1, d, 0))*w1;
    Expr c2 = sum(select(r.y ==2, d, 0))*w2;
    Expr c3 = sum(select(r.y ==3, d, 0))*w3;

    Expr value = c0 + c1 + c2 + c3;
    value = select(value > cast<float>(type_of<T>().max()), cast<float>(type_of<T>().max()),
                   value < cast<float>(type_of<T>().min()), cast<float>(type_of<T>().min()),
                   value + 0.5f);
    dst(x, y) = cast<T>(value);
    return dst;
}

template <typename T>
Func subimage(Func src, Expr originx, Expr originy)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    dst(x, y) = src(x+originx, y+originy);
    return dst;
}

Func label_firstpass(Func src, int32_t width, int32_t height)
{
    Func original{"original"};
    Expr MAX = width*height+1;
    Var x{"org_x"}, y{"org_y"};
    original(x, y) = cast<int32_t>(select(src(x,y)==0, MAX, x+y*width+1));

    Func extend = BoundaryConditions::constant_exterior(original, MAX, 0, width, 0, height);
    Func copy{"copy"};

    Func findMin{"findMin"};
    findMin(x, y) = extend(x, y);

    RDom s{0, width, 0, height, "s"};
    findMin(s.x, s.y) = select(findMin(s.x, s.y)== MAX,
                                    findMin(s.x, s.y),
                                    min(findMin(s.x, s.y),
                                        min(min(findMin(s.x-1, s.y-1),
                                                findMin(s.x,   s.y-1)),
                                            min(findMin(s.x+1, s.y-1),
                                                findMin(s.x-1, s.y  )))));
    findMin.compute_root();

    Func backward{"back"}, whereUp{"where"};
    whereUp(x, y) = select(findMin(x, y)== MAX, MAX,
                                                min(min(findMin(x+1, y  ),
                                                        findMin(x-1, y+1)),
                                                    min(findMin(x  , y+1),
                                                        findMin(x+1, y+1))));
    whereUp(x, y) = select(whereUp(x, y) < findMin(x, y), whereUp(x, y), MAX);
    whereUp.compute_root();

    backward(x, y) = findMin(x, y);
    Expr indX{"indx"},indY{"indy"};
    indX = (findMin(s.x, s.y)-1)%width;
    indY = select((findMin(s.x, s.y)-1)/width < height, (findMin(s.x, s.y)-1)/width, height-1);
    Expr target = backward(clamp(indX, 0, width), clamp(indY, 0, height));

    Expr tarX = (whereUp(s.x, s.y)-1)%width;
    Expr tarY = select((whereUp(s.x, s.y)-1)/width < height, (whereUp(s.x, s.y)-1)/width, height-1);
    Expr origin = backward(clamp(tarX, 0, width), clamp(tarY, 0, height));

    backward(clamp(indX, 0, width), clamp(indY, 0, height)) =
        select( target> whereUp(s.x, s.y), min(whereUp(s.x, s.y), origin),
               target);
    backward.compute_root();

    Func final;
    final(x, y) = backward(x, y);
    final(s.x, s.y) = select(final(s.x, s.y)== MAX,
                                     final(s.x, s.y),
                                     min(final(s.x, s.y),
                                         min(min(final(s.x-1, s.y-1),
                                                 final(s.x,   s.y-1)),
                                             min(final(s.x+1, s.y-1),
                                                 final(s.x-1, s.y  )))));
    final.compute_root();

    Func firstPass{"firstPass"};
    copy(x, y) = select(final(x, y) == MAX, 0, final(x, y));
    firstPass(x, y) = Tuple(copy(x, y),
                            select(copy(x, y) ==0, 0,
                                   copy(x,y) < copy(x-1, y-1) ||
                                   copy(x,y) < copy(x  , y-1) ||
                                   copy(x,y) < copy(x+1, y-1) ||
                                   copy(x,y) < copy(x-1, y),
                                    1, 0));
    //firstPass.print_loop_nest();
    return firstPass;
}

Func label_secondpass(Func src, Func buf, int32_t width, int32_t height, Expr bufW){
    Func secondPass;
    Var x, y;
    RDom r{0, bufW, "r"};

    secondPass(x, y) = src(x, y);
    secondPass(x, y) = select(src(x, y)!=0&&buf(r.x, 0) == secondPass(x, y),
                                    buf(r.x, 1),
                                    secondPass(x, y));

    // secondPass.print_loop_nest();
    return secondPass;
}

} // anonymous

} // Element
} // Halide
