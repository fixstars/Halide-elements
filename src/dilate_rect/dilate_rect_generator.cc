#include <cstdint>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class DilateRect : public Halide::Generator<DilateRect<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func output{"output"};

        output = Element::dilate_rect<T>(input, width, height, window_width, window_height, iteration);

        schedule(input, {width, height});
        schedule(output, {width, height});

        return output;
    }
};

HALIDE_REGISTER_GENERATOR(DilateRect<uint8_t>, dilate_rect_u8);
HALIDE_REGISTER_GENERATOR(DilateRect<uint16_t>, dilate_rect_u16);
