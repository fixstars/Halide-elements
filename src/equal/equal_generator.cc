#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Equal : public Halide::Generator<Equal<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};
    Param<int32_t> width{"width", 1024};
    Param<int32_t> height{"height", 768};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
        Expr dstval = cast<T>(select(srcval0 == srcval1, type_of<T>().max(), 0));
        dst(x, y) = dstval;

        return dst;
    }
};

RegisterGenerator<Equal<uint8_t>> equal_u8{"equal_u8"};
RegisterGenerator<Equal<uint16_t>> equal_u16{"equal_u16"};
RegisterGenerator<Equal<uint32_t>> equal_u32{"equal_u32"};
