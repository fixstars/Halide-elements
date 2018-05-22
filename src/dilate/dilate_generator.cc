#include <cstdint>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Dilate : public Halide::Generator<Dilate<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};
    ImageParam structure{UInt(8), 2, "structure"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func output{"output"};

        output = Element::dilate<T>(input, width, height, window_width, window_height, structure, iteration);

        schedule(input, {width, height});
        schedule(structure, {window_width, window_height});
        schedule(output, {width, height});

        return output;
    }
};

HALIDE_REGISTER_GENERATOR(Dilate<uint8_t>, dilate_u8);
HALIDE_REGISTER_GENERATOR(Dilate<uint16_t>, dilate_u16);
