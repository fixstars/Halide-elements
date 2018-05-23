#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Median : public Halide::Generator<Median<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};

    Func build() {
        Func dst{"dst"};

        dst = Element::median(src, width, height, window_width, window_height);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Median<uint8_t>, median_u8);
HALIDE_REGISTER_GENERATOR(Median<uint16_t>, median_u16);
