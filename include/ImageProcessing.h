#pragma once

#include <Halide.h>
#include "Element.h"
#include "Schedule.h"

namespace Halide {
namespace Element {

Func affine(Func in, int32_t width, int32_t height, Param<float> degrees, 
            Param<float> scale_x, Param<float> scale_y,
            Param<float> shift_x, Param<float> shift_y, Param<float> skew_y) 
{
    Var x, y;
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

    Func tx("tx"), ty("ty");
    tx(x,y) = cast<int>((a00*x + a10*y + a20) / det);
    ty(x,y) = cast<int>((a01*x + a11*y + a21) / det);

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

} // Element    
} // Halide