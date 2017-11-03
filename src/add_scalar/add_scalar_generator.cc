#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;

template<typename T>
class Add : public Halide::Generator<Add<T>> {
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 1, "src1"};

public:
    Func build() {
        return Element::add_scalar<T>(src0, src1(0));
    }
};

RegisterGenerator<Add<uint8_t>> add_u8{"add_scalar_u8"};
RegisterGenerator<Add<uint16_t>> add_u16{"add_scalar_u16"};
RegisterGenerator<Add<uint32_t>> add_u32{"add_scalar_u32"};
