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

RegisterGenerator<SubScalar<uint8_t>> sub_scalar_u8{"sub_scalar_u8"};
RegisterGenerator<SubScalar<uint16_t>> sub_scalar_u16{"sub_scalar_u16"};
RegisterGenerator<SubScalar<uint32_t>> sub_scalar_u32{"sub_scalar_u32"};
