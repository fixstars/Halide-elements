#include <cstdint>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class MaxPos : public Halide::Generator<MaxPos<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::max_pos(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {2});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(MaxPos<uint8_t>, max_pos_u8);
HALIDE_REGISTER_GENERATOR(MaxPos<uint16_t>, max_pos_u16);
HALIDE_REGISTER_GENERATOR(MaxPos<uint32_t>, max_pos_u32);
HALIDE_REGISTER_GENERATOR(MaxPos<float>, max_pos_f32);
HALIDE_REGISTER_GENERATOR(MaxPos<double>, max_pos_f64);
