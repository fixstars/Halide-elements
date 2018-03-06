#pragma once

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

    schedule(in, {width, height});
    schedule(tx, {width, height});
    schedule(ty, {width, height});
    schedule(affine, {width, height});

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

    schedule(in, {width, height});
    kernel.compute_root();
    kernel.bound(x, -(window_width / 2), window_width);
    kernel.bound(y, -(window_height / 2), window_height);
    schedule(kernel_sum, {1});
    schedule(dst, {width, height});

    return dst;
}

Func convolution(Func in, int32_t width, int32_t height, Func kernel, int32_t kernel_size, int32_t unroll_factor) {
    Var x, y;

    Func bounded = BoundaryConditions::repeat_edge(in, 0, width, 0, height);

    Expr kh = Halide::div_round_to_zero(kernel_size, 2);
    RDom r(0, kernel_size, 0, kernel_size);

    Expr dx = r.x - kh;
    Expr dy = r.y - kh;

    Func k;
    k(x, y) = kernel(x, y);

    constexpr uint32_t frac_bits = 10;
    using Fixed16 = Fixed<int16_t, frac_bits>;
    Fixed16 pv = to_fixed<int16_t, frac_bits>(bounded(x+dx, y+dy));
    Fixed16 kv{k(r.x, r.y)};

    Func out("out");
    out(x, y) = from_fixed<uint8_t>(sum_unroll(r, pv * kv));

    schedule(in, {width, height});
    schedule(kernel, {5, 5});
    schedule(k, {5, 5});
    schedule(out, {width, height}).unroll(x, unroll_factor);

    return out;
}

template<uint32_t frac_bits>
Func gamma_correction(Func in, Param<float> value)
{
    Var x, y, c;

    Fixed<int16_t, frac_bits>  v = Fixed<int16_t, frac_bits>{in(c,x,y)};
    Func out;
    out(c,x,y) = static_cast<Expr>(to_fixed<int16_t, frac_bits>(fast_pow(from_fixed<float>(v), value)));
    return out;
}

Func optical_black_clamp(Func in, Param<uint16_t> clamp_value)
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
Func saturation_adjustment(Func in, Param<float> value)
{
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

            schedule(next, {size, width, height});
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

    schedule(input_f, {width, height});
    schedule(diff_x, {width, height});
    schedule(diff_y, {width, height});
    schedule(output, {width, height});

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
Func split3(Func src, int32_t widthe, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    dst(x, y) = Tuple(src(0, x, y), src(1, x, y), src(2, x, y));

    return dst;
}

} //anonoymous
} // Element
} // Halide
