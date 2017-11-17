#include <iostream>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class DilateCross : public Halide::Generator<DilateCross<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam input{type_of<T>(), 2, "input"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

	Func build() {
		Func output("output");
        output(x, y) = dilate_cross<T>(lambda(_, input(_)), width, height, window_width, window_height, iteration)(x, y);
        schedule(input, {width, height});
        schedule(output, {width, height});
        return output;
    }
};

RegisterGenerator<DilateCross<uint8_t>> dilate_cross_u8{"dilate_cross_u8"};
RegisterGenerator<DilateCross<uint16_t>> dilate_cross_u16{"dilate_cross_u16"};
