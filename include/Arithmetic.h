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

    Func dst("dst");
    Expr srcval0 = cast<uint64_t>(src0(x, y)), srcval1 = cast<uint64_t>(src1(x, y));
    
    Expr dstval = min(srcval0 + srcval1, cast<uint64_t>(type_of<T>().max()));
    
    dst(x, y) = cast<T>(dstval);
    
    return dst;
}


template<typename T>
Halide::Func add_scalar(Halide::Func src0, Halide::Expr val)
{
    using namespace Halide;

    Var x, y;

    Func dst("dst");
    Expr srcval0 = cast<uint64_t>(src0(x, y)), srcval1 = cast<uint64_t>(val);
    
    Expr dstval = min(srcval0 + srcval1, cast<uint64_t>(type_of<T>().max()));
    
    dst(x, y) = cast<T>(dstval);
    
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

Func max_pos(Func in, int32_t width, int32_t height) {
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
