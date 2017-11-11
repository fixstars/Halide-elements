#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;

template<typename T>
class AndScalar : public Halide::Generator<AndScalar<T>> {
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 1, "src1"};

public:
    Func build() {
        return Element::and_scalar<T>(src0, src1(0));
    }
};

RegisterGenerator<AndScalar<uint8_t>> and_scalar_u8{"and_scalar_u8"};
RegisterGenerator<AndScalar<uint16_t>> and_scalar_u16{"and_scalar_u16"};
RegisterGenerator<AndScalar<uint32_t>> and_scalar_u32{"and_scalar_u32"};
