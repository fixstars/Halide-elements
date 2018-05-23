#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class AddScalar : public Halide::Generator<AddScalar<T>> {
    ImageParam src{type_of<T>(), 2, "src"};
    Param<double> value{"value", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

public:
    Func build() {
        Func dst{"dst"};

        dst = Element::add_scalar<T>(src, value);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(AddScalar<uint8_t>, add_scalar_u8);
HALIDE_REGISTER_GENERATOR(AddScalar<uint16_t>, add_scalar_u16);
HALIDE_REGISTER_GENERATOR(AddScalar<uint32_t>, add_scalar_u32);
