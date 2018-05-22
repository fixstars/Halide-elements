#include <cstdint>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Average : public Halide::Generator<Average<T>> {
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};

public:
    Func build() {
        Func dst{"dst"};

        dst = Element::average<T>(src, window_width, window_height);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Average<uint8_t>, average_u8);
HALIDE_REGISTER_GENERATOR(Average<uint16_t>, average_u16);
