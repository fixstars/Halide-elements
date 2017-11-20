#ifndef HALIDE_ELEMENT_ARITHMETIC_H
#define HALIDE_ELEMENT_ARITHMETIC_H

#include "Halide.h"
#include "Util.h"

namespace Halide {
namespace Element {

template<typename T>
Halide::Func add(Halide::Func src0, Halide::Func src1)
{
    using namespace Halide;
    using upper_t = typename Upper<T>::type;

    Var x, y;

    Func dst;
    Expr srcval0 = cast<upper_t>(src0(x, y)), srcval1 = cast<upper_t>(src1(x, y));

    Expr dstval = min(srcval0 + srcval1, cast<upper_t>(type_of<T>().max()));
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
    dst(x, y) = src0(x, y) & src1(x, y);

    return dst;
}

template<typename T>
Halide::Func and_scalar(Halide::Func src0, Halide::Expr val)
{
    using namespace Halide;

    Var x, y;

    Func dst;
    dst(x, y) = src0(x, y) & val;

    return dst;
}

template<typename T>
Halide::Func average(Halide::ImageParam src, int32_t window_width, int32_t window_height)
{
    using namespace Halide;
    using upper_t = typename Upper<T>::type;

    Var x, y;

    Func clamped = BoundaryConditions::repeat_edge(src);
    Expr w_half = div_round_to_zero(window_width, 2);
    Expr h_half = div_round_to_zero(window_height, 2);

    RDom r(-w_half, window_width, -h_half, window_height);

    Func dst;
    dst(x, y) = cast<T>(round(cast<float>(sum(cast<upper_t>(clamped(x + r.x, y + r.y)))) / cast<float>(window_width * window_height)) + 0.5f);

    return dst;
}

template <typename T>
Halide::Func multiply(Halide::Func src1, Halide::Func src2) {
    using namespace Halide;

    Var x, y;
    
    Func dst("dst");
    dst(x, y) = src1(x, y) * src2(x, y);
    
    return dst;
}

template <typename T>
Halide::Func mul_scalar(Halide::Func src0, Halide::Expr val) {
    using namespace Halide;

    Var x, y;
    
    Func dst("dst");

    Expr srcval = src0(x, y);
    Expr dstval = min(srcval * val, cast<float>(type_of<T>().max()));
    dstval = max(dstval, 0);
    dst(x, y) = cast<T>(round(dstval));

    return dst;
}

template <typename T>
Halide::Func div_scalar(Halide::Func src, Halide::Expr val) {
    Var x, y;
    
    Func dst("dst");

    Expr srcval = src(x, y);
    Expr dstval = min(srcval / val, cast<double>(type_of<T>().max()));
    dstval = max(dstval, 0);
    dst(x, y) = cast<T>(round(dstval));

    return dst;
}

template <typename T>
Halide::Func nand(Halide::Func src0, Halide::Func src1) {
    using namespace Halide;

    Var x, y;
    
    Func dst("dst");
    dst(x, y) = ~(src0(x, y) & src1(x, y));
    
    return dst;
}

template <typename T>
Halide::Func nor(Halide::Func src0, Halide::Func src1) {
    using namespace Halide;

    Var x, y;
    
    Func dst("dst");
    dst(x, y) = ~(src0(x, y) | src1(x, y));
    
    return dst;
}

template <typename T>
Halide::Func equal(Halide::Func src0, Halide::Func src1) {
    Var x, y;
    
    Func dst("dst");    
    Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
    Expr dstval = cast<T>(select(srcval0 == srcval1, type_of<T>().max(), 0));
    dst(x, y) = dstval;

    return dst;
}

template <typename T>
Halide::Func cmpgt(Halide::Func src0, Halide::Func src1) {
    Var x, y;
    
    Func dst("dst");
    Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
    Expr dstval = cast<T>(select(srcval0 > srcval1, type_of<T>().max(), 0));
    dst(x, y) = dstval;
    
    return dst;
}

template <typename T>
Halide::Func cmpge(Halide::Func src0, Halide::Func src1) {
    Var x, y;
    
    Func dst("dst");
    Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
    Expr dstval = cast<T>(select(srcval0 >= srcval1, type_of<T>().max(), 0));;
    dst(x, y) = dstval;

    return dst;
}

}
}

#endif
