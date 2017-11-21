#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class ErodeRect : public Halide::Generator<ErodeRect<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

    Func build() {
       return erode_rect<T>(src, width, height, window_width, window_height, iteration);
    }
};

RegisterGenerator<ErodeRect<uint8_t>> erode_rect_u8{"erode_rect_u8"};
RegisterGenerator<ErodeRect<uint16_t>> erode_rect_u16{"erode_rect_u16"};
