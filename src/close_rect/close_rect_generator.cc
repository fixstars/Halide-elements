#include <cstdint>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class CloseRect : public Halide::Generator<CloseRect<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func output{"output"};

        output = Element::close_rect<T>(input, width, height, window_width, window_height, iteration);

        schedule(input, {width, height});
        schedule(output, {width, height});

        return output;
    }
};

RegisterGenerator<CloseRect<uint8_t>> close_rect_u8{"close_rect_u8"};
RegisterGenerator<CloseRect<uint16_t>> close_rect_u16{"close_rect_u16"};
