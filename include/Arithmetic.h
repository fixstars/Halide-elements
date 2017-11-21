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

Halide::Func calc_and(Halide::Func src0, Halide::Func src1)
{
    using namespace Halide;

    Var x, y;

    Func dst;
    dst(x, y) = src0(x, y) & src1(x, y);

    return dst;
}

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

Halide::Func multiply(Halide::Func src1, Halide::Func src2) {
    using namespace Halide;

    Var x, y;

    Func dst("dst");
    dst(x, y) = src1(x, y) * src2(x, y);

    return dst;
}

template<typename T>
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

template<typename T>
Halide::Func div_scalar(Halide::Func src, Halide::Expr val) {
    Var x, y;

    Func dst("dst");

    Expr srcval = src(x, y);
    Expr dstval = min(srcval / val, cast<double>(type_of<T>().max()));
    dstval = max(dstval, 0);
    dst(x, y) = cast<T>(round(dstval));

    return dst;
}

template<typename T>
Halide::Func nand(Halide::Func src0, Halide::Func src1) {
    using namespace Halide;

    Var x, y;

    Func dst("dst");
    dst(x, y) = ~(src0(x, y) & src1(x, y));

    return dst;
}

template<typename T>
Halide::Func nor(Halide::Func src0, Halide::Func src1) {
    using namespace Halide;

    Var x, y;

    Func dst("dst");
    dst(x, y) = ~(src0(x, y) | src1(x, y));

    return dst;
}

Halide::Func min(Halide::Func src0, Halide::Func src1)
{
    Var x, y;

    Func dst("dst");
    dst(x, y) = min(src0(x, y), src1(x, y));

    return dst;
}

Halide::Func max(Halide::Func src0, Halide::Func src1)
{
    Var x, y;

    Func dst("dst");
    dst(x, y) = max(src0(x, y), src1(x, y));

    return dst;
}

Halide::Func max_pos(Func in, int32_t width, int32_t height) {
    Func dst("dst");
    RDom r(0, width, 0, height, "r");
    Func res("res");
    Var x("x");
    res(x) = argmax(r, in(r.x, r.y));
    schedule(res, {1});

    Var d("d");
    dst(d) = cast<uint32_t>(0);
    dst(0) = cast<uint32_t>(res(0)[0]);
    dst(1) = cast<uint32_t>(res(0)[1]);
}

template<typename T>
Halide::Func equal(Halide::Func src0, Halide::Func src1) {
    Var x, y;

    Func dst("dst");
    dst(x, y) = cast<T>(select(src0(x, y) == src1(x, y), type_of<T>().max(), 0));

    return dst;
}

template<typename T>
Halide::Func cmpgt(Halide::Func src0, Halide::Func src1) {
    Var x, y;

    Func dst("dst");
    dst(x, y) = cast<T>(select(src0(x, y) > src1(x, y), type_of<T>().max(), 0));

    return dst;
}

template<typename T>
Halide::Func cmpge(Halide::Func src0, Halide::Func src1) {
    Var x, y;

    Func dst("dst");
    dst(x, y) = cast<T>(select(src0(x, y) >= src1(x, y), type_of<T>().max(), 0));;

    return dst;
}

template<typename T>
Func max_value(Func in, Func roi, int32_t width, int32_t height) {
    Func count("count"), dst("dst");

    RDom r(0, width, 0, height, "r");
    r.where(roi(r.x, r.y) != 0);

    Var x{"x"};
    count(x) = sum(select(roi(r.x, r.y) == 0, 0, 1));

    dst(x) = cast<T>(select(count(x) == 0, 0, maximum(in(r.x, r.y))));

    schedule(count, {1});
    return dst;
}

template<typename T>
Func integral(Func in, int32_t width, int32_t height) {
    Var x("x"), y("y");
    Func dst("dst"), integral("integral");
    integral(x, y) = cast<uint64_t>(in(x, y));

    RDom r1(1, width - 1, 0, height, "r1");
    integral(r1.x, r1.y) += integral(r1.x - 1, r1.y);

    RDom r2(0, width, 1, height - 1, "r2");
    integral(r2.x, r2.y) += integral(r2.x, r2.y - 1);
    schedule(integral, {width, height});
    dst(x, y) = cast<T>(integral(x, y));

    return dst;
}

}
}

#endif
