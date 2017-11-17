#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Erode : public Halide::Generator<Erode<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        return erode<T>(src, width, height, window_width, window_height, structure, iteration);
    }
};

RegisterGenerator<Erode<uint8_t>> erode_u8{"erode_u8"};
RegisterGenerator<Erode<uint16_t>> erode_u16{"erode_u16"};
