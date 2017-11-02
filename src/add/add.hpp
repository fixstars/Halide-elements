#include <iostream>
#include <limits>
#include "Halide.h"

using namespace Halide;

template<typename T>
Func add(Func src0, Func src1)
{
    Var x, y;

    Func dst("dst");
    Expr srcval0 = cast<uint64_t>(src0(x, y)), srcval1 = cast<uint64_t>(src1(x, y));
    
    Expr dstval = min(srcval0 + srcval1, cast<uint64_t>(type_of<T>().max()));
    
    dst(x, y) = cast<T>(dstval);
    
    return dst;
}