#include <iostream>
#include <limits>
#include "Halide.h"

#include "add.hpp"

using namespace Halide;

/*
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
*/

template<typename T>
class Add : public Halide::Generator<Add<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};
    
    //Var x, y;
    
    Func build() {
        
        return add<T>(src0, src1);
        /*
        Func dst("dst");
        Expr srcval0 = cast<uint64_t>(src0(x, y)), srcval1 = cast<uint64_t>(src1(x, y));
        
        Expr dstval = min(srcval0 + srcval1, cast<uint64_t>(type_of<T>().max()));
        
        dst(x, y) = cast<T>(dstval);
        
        return dst;
        */
    }
};

RegisterGenerator<Add<uint8_t>> add_u8{"add_u8"};
RegisterGenerator<Add<uint16_t>> add_u16{"add_u16"};
RegisterGenerator<Add<uint32_t>> add_u32{"add_u32"};


