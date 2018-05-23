#include <cstdint>

#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Open : public Halide::Generator<Open<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func erode{"erode"}, dilate{"dilate"};

        erode = Element::erode<T>(src, width, height, window_width, window_height, structure, iteration);
        dilate = Element::dilate<T>(erode, width, height, window_width, window_height, structure, iteration);

        schedule(src, {width, height});
        schedule(structure, {window_width, window_height});
        schedule(erode, {width, height});
        schedule(dilate, {width, height});

        return dilate;
    }
};

HALIDE_REGISTER_GENERATOR(Open<uint8_t>, open_u8);
HALIDE_REGISTER_GENERATOR(Open<uint16_t>, open_u16);
