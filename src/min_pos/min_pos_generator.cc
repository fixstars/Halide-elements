#include <cstdint>

#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class MinPos : public Halide::Generator<MinPos<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::min_pos(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {2});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(MinPos<uint8_t>, min_pos_u8);
HALIDE_REGISTER_GENERATOR(MinPos<uint16_t>, min_pos_u16);
HALIDE_REGISTER_GENERATOR(MinPos<uint32_t>, min_pos_u32);
HALIDE_REGISTER_GENERATOR(MinPos<float>, min_pos_f32);
HALIDE_REGISTER_GENERATOR(MinPos<double>, min_pos_f64);
