#include <cstdint>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class MaxValue : public Halide::Generator<MaxValue<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam roi{type_of<uint8_t>(), 2, "roi"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::max_value<T>(src, roi, width, height);

        schedule(src, {width, height});
        schedule(roi, {width, height});
        schedule(dst, {1});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(MaxValue<uint8_t>, max_value_u8);
HALIDE_REGISTER_GENERATOR(MaxValue<uint16_t>, max_value_u16);
HALIDE_REGISTER_GENERATOR(MaxValue<uint32_t>, max_value_u32);
