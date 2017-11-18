#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class OpenRect : public Halide::Generator<OpenRect<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func erode = conv_rect(src, [](RDom r, Expr e){return minimum_unroll(r, e);}, width, height, iteration, window_width, window_height);
        Func dilate = conv_rect(erode, [](RDom r, Expr e){return maximum_unroll(r, e);}, width, height, iteration, window_width, window_height);

        schedule(src, {width, height});
        
        return dilate;
    }
};

RegisterGenerator<OpenRect<uint8_t>> open_rect_u8{"open_rect_u8"};
RegisterGenerator<OpenRect<uint16_t>> open_rect_u16{"open_rect_u16"};
