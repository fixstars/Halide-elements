#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class SubScalar : public Halide::Generator<SubScalar<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<double> value{"value", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::sub_scalar<T>(src, value);

        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(SubScalar<uint8_t>, sub_scalar_u8);
HALIDE_REGISTER_GENERATOR(SubScalar<uint16_t>, sub_scalar_u16);
HALIDE_REGISTER_GENERATOR(SubScalar<uint32_t>, sub_scalar_u32);
