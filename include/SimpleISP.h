#pragma once

#include <Halide.h>
#include "Element.h"

namespace Halide {
namespace Element {

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

Func simple_isp(Func in, Param<uint16_t> optical_black_value, 
                Param<float> gamma_value, Param<float> saturation_value, 
                int32_t width, int32_t height) 
{
    constexpr uint32_t frac_bits = 10;
    Var c, x, y;
    Func f0("optical_black_clamp");
    f0(x, y) = optical_black_clamp(in, optical_black_value)(x, y);

    Func f0_ = BoundaryConditions::mirror_interior(f0, 0, width, 0, height);

    Func f1("color_interpolation_raw2rgb");
    f1(c, x, y) = color_interpolation_raw2rgb<frac_bits>(f0_)(c, x, y);

    Func f2("gamma_correction");
    f2(c, x, y) = gamma_correction<frac_bits>(f1, gamma_value)(c,x,y);

    Func f3("color_interpolation_rgb2hsv");
    f3(c, x, y) = color_interpolation_rgb2hsv<frac_bits>(f2)(c, x, y);

    Func f4("saturation_adjustment");
    f4(c, x, y) = saturation_adjustment<frac_bits>(f3, saturation_value)(c, x, y);

    Func f5("color_interpolation_hsv2rgb");
    f5(c, x, y) = color_interpolation_hsv2rgb<frac_bits>(f4)(c, x, y);

    Func out("out");
    out(c, x, y) = select(c == 3, 0, denormalize<frac_bits>(f5)(c, x, y));

    schedule(in, {width, height});
    schedule(f2, {3, width, height}).unroll(c);
    schedule(f4, {3, width, height}).unroll(c);
    schedule(out, {4, width, height}).unroll(c);
    
    return out;
}

} // Element
} // Halide