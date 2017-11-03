#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;

template<typename T>
class And : public Halide::Generator<And<T>> {
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

public:
    Func build() {
        return Element::calc_and<T>(src0, src1);
    }
};

RegisterGenerator<And<uint8_t>> and_u8{"and_u8"};
RegisterGenerator<And<uint16_t>> and_u16{"and_u16"};
RegisterGenerator<And<uint32_t>> and_u32{"and_u32"};
