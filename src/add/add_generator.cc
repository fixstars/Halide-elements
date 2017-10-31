#include <iostream>
#include <limits>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Add : public Halide::Generator<Add<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};
    //ImageParam src{type_of<T>(), 2, "src"};
    //Param<double> value{"value", 2.0};
    
    Var x, y;
    
    Func build() {
        
        Func dst("dst");
        Expr srcval0 = cast<uint64_t>(src0(x, y)), srcval1 = cast<uint64_t>(src1(x, y));
        
        Expr dstval = min(srcval0 + srcval1, cast<uint64_t>(type_of<T>().max()));
        //Expr dstval = min(srcval / value, cast<double>(type_of<T>().max()));
        dst(x, y) = cast<T>(dstval);
        
        return dst;
    }
};

RegisterGenerator<Add<uint8_t>> add_u8{"add_u8"};
RegisterGenerator<Add<uint16_t>> add_u16{"add_u16"};
RegisterGenerator<Add<uint32_t>> add_u32{"add_u32"};



/** orginal
 
 template<typename T>
 class DivScalar : public Halide::Generator<DivScalar<T>> {
 public:
 GeneratorParam<int32_t> width{"width", 1024};
 GeneratorParam<int32_t> height{"height", 768};
 ImageParam src{type_of<T>(), 2, "src"};
 Param<double> value{"value", 2.0};
 
 Var x, y;
 
 Func build() {
 
 Func dst("dst");
 Expr srcval = src(x, y);
 Expr dstval = min(srcval / value, cast<double>(type_of<T>().max()));
 dstval = max(dstval, 0);
 dst(x, y) = cast<T>(round(dstval));
 
 return dst;
 }
 };
 
 RegisterGenerator<DivScalar<uint8_t>> div_scalar_u8{"div_scalar_u8"};
 RegisterGenerator<DivScalar<uint16_t>> div_scalar_u16{"div_scalar_u16"};
 RegisterGenerator<DivScalar<uint32_t>> div_scalar_u32{"div_scalar_u32"};
 
 */
// test
/*
class Add : public Halide::Generator<Add> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{UInt(8), 2, "src0"}, src1{UInt(8), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval0 = cast<uint16_t>(src0(x, y)), srcval1 = cast<uint16_t>(src1(x, y));
	
        Expr dstval = cast<uint8_t>(min(srcval0 + srcval1, cast<uint16_t>(255)));
        dst(x, y) = dstval;

        return dst;
    }
};

RegisterGenerator<Add> add{"add"};
 
 */
