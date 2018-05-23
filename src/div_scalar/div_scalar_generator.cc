#include <cstdint>

#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class DivScalar : public Halide::Generator<DivScalar<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<double> value{"value", 2.0};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst("dst");

        dst = Element::div_scalar<T>(src, value);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(DivScalar<uint8_t>, div_scalar_u8);
HALIDE_REGISTER_GENERATOR(DivScalar<uint16_t>, div_scalar_u16);
HALIDE_REGISTER_GENERATOR(DivScalar<uint32_t>, div_scalar_u32);
