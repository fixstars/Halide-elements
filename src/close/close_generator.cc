#include <cstdint>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Close : public Halide::Generator<Close<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};
    ImageParam structure{UInt(8), 2, "structure"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func erode{"erode"}, dilate{"dilate"};

        dilate = Element::dilate<T>(input, width, height, window_width, window_height, structure, iteration);
        erode = Element::erode<T>(dilate, width, height, window_width, window_height, structure, iteration);

        schedule(input, {width, height});
        schedule(structure, {window_width, window_height});
        schedule(dilate, {width, height});
        schedule(erode, {width, height});

        return erode;
    }
};

HALIDE_REGISTER_GENERATOR(Close<uint8_t>, close_u8);
HALIDE_REGISTER_GENERATOR(Close<uint16_t>, close_u16);
