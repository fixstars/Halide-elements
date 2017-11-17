#ifndef HALIDE_ELEMENT_ARITHMETIC_H
#define HALIDE_ELEMENT_ARITHMETIC_H

#include "Halide.h"

namespace Halide {
namespace Element {

template<typename T>
Halide::Func add(Halide::Func src0, Halide::Func src1)
{
    using namespace Halide;

    Var x, y;

    Func dst;
    Expr srcval0 = cast<uint64_t>(src0(x, y)), srcval1 = cast<uint64_t>(src1(x, y));

    Expr dstval = min(srcval0 + srcval1, cast<uint64_t>(type_of<T>().max()));

    dst(x, y) = cast<T>(dstval);

    return dst;
}


template<typename T>
Halide::Func add_scalar(Halide::Func src, Halide::Expr val)
{
    using namespace Halide;

    Var x, y;

    Func dst;

    Expr dstval = clamp(round(cast<double>(src(x, y)) + val), 0, cast<double>(type_of<T>().max()));
    dst(x, y) = cast<T>(dstval);

    return dst;
}

template<typename T>
Halide::Func calc_and(Halide::Func src0, Halide::Func src1)
{
    using namespace Halide;

    Var x, y;

    Func dst;
    Expr srcval0 = src0(x, y), srcval1 = src1(x, y);

    Expr dstval = srcval0 & srcval1;

    dst(x, y) = dstval;

    return dst;
}

template<typename T>
Halide::Func and_scalar(Halide::Func src0, Halide::Expr val)
{
    using namespace Halide;

    Var x, y;

    Func dst;
    Expr srcval0 = src0(x, y), srcval1 = val;

    Expr dstval = srcval0 & srcval1;

    dst(x, y) = dstval;

    return dst;
}

template<typename T>
Halide::Func average(Halide::ImageParam src, int32_t window_width, int32_t window_height)
{
    using namespace Halide;

    Var x, y;

    Func clamped = BoundaryConditions::repeat_edge(src);
    Expr w_half = div_round_to_zero(window_width, 2);
    Expr h_half = div_round_to_zero(window_height, 2);

    RDom r(-w_half, window_width, -h_half, window_height);


    Func dst;

    Expr dstval = cast<T>(round(sum(cast<float>(clamped(x + r.x, y + r.y))) / cast<float>(window_width * window_height)));

    dst(x, y) = dstval;

    return dst;
}



}
}

#endif
