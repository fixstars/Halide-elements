#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class OpenCross : public Halide::Generator<OpenCross<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Func build() {
        Func erode = conv_cross(src, [](RDom r, Expr e){return minimum_unroll(r, e);}, width, height, iteration, window_width, window_height);
        Func dilate = conv_cross(erode, [](RDom r, Expr e){return maximum_unroll(r, e);}, width, height, iteration, window_width, window_height);
        schedule(src, {width, height});
        
        return dilate;
    }
};

RegisterGenerator<OpenCross<uint8_t>> open_cross_u8{"open_cross_u8"};
RegisterGenerator<OpenCross<uint16_t>> open_cross_u16{"open_cross_u16"};
