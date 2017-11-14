#include <iostream>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Dilate : public Halide::Generator<Dilate<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam input{type_of<T>(), 2, "input"};
    ImageParam structure{UInt(8), 2, "structure"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

	Func build() {
		Func output("output");
        output(x, y) = dilate<T>(lambda(_, input(_)), width, height, window_width, window_height, lambda(_, structure(_)), iteration)(x, y);
        schedule(input, {width, height});
        schedule(structure, {window_width, window_height});
        schedule(output, {width, height});
        return output;
    }
};

RegisterGenerator<Dilate<uint8_t>> dilate_u8{"dilate_u8"};
RegisterGenerator<Dilate<uint16_t>> dilate_u16{"dilate_u16"};
