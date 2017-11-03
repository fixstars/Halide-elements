#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;

template<typename T>
class AddScalar : public Halide::Generator<AddScalar<T>> {
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 1, "src1"};

public:
    Func build() {
        return Element::add_scalar<T>(src0, src1(0));
    }
};

RegisterGenerator<AddScalar<uint8_t>> add_scalar_u8{"add_scalar_u8"};
RegisterGenerator<AddScalar<uint16_t>> add_scalar_u16{"add_scalar_u16"};
RegisterGenerator<AddScalar<uint32_t>> add_scalar_u32{"add_scalar_u32"};
