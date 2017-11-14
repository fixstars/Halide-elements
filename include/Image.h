#pragma once

#include <Halide.h>
#include "Element.h"
#include "Schedule.h"

namespace Halide {
namespace Element {

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