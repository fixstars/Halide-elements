#include <cstdint>

#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class MulScalar : public Halide::Generator<MulScalar<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<float> value{"value", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::mul_scalar<T>(src, value);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(MulScalar<uint8_t>, mul_scalar_u8);
HALIDE_REGISTER_GENERATOR(MulScalar<uint16_t>, mul_scalar_u16);
HALIDE_REGISTER_GENERATOR(MulScalar<uint32_t>, mul_scalar_u32);
