#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class ErodeCross : public Halide::Generator<ErodeCross<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        return erode_cross<T>(src, width, height, window_width, window_height, iteration);
    }
};

RegisterGenerator<ErodeCross<uint8_t>> erode_cross_u8{"erode_cross_u8"};
RegisterGenerator<ErodeCross<uint16_t>> erode_cross_u16{"erode_cross_u16"};
