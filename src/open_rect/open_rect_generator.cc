#include <cstdint>

#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class OpenRect : public Halide::Generator<OpenRect<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func erode{"erode"}, dilate{"dilate"};

        erode = Element::erode_rect<T>(src, width, height, window_width, window_height, iteration);
        dilate = Element::dilate_rect<T>(erode, width, height, window_width, window_height, iteration);

        schedule(src, {width, height});
        schedule(erode, {width, height});
        schedule(dilate, {width, height});

        return dilate;
    }
};

HALIDE_REGISTER_GENERATOR(OpenRect<uint8_t>, open_rect_u8);
HALIDE_REGISTER_GENERATOR(OpenRect<uint16_t>, open_rect_u16);
