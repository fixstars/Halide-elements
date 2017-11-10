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


}
}

#endif
