#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Nor : public Halide::Generator<Nor<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr dstval = ~(src0(x, y) | src1(x, y));
        dst(x, y) = dstval;

        return dst;
    }
};

RegisterGenerator<Nor<uint8_t>> nor_u8{"nor_u8"};
RegisterGenerator<Nor<uint16_t>> nor_u16{"nor_u16"};
RegisterGenerator<Nor<uint32_t>> nor_u32{"nor_u32"};
