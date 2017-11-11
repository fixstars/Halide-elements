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
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

	Func build() {
		Func input("input");
		input(x, y) = src(x, y);

		Func structure_("structure_");
		structure_(x, y) = structure(x, y);
		schedule(structure, {window_width, window_height});
		schedule(structure_, {window_width, window_height});

		return dilate<T>(input,width,height,window_width,window_height,structure_,iteration);
    }
};

RegisterGenerator<Dilate<uint8_t>> dilate_u8{"dilate_u8"};
RegisterGenerator<Dilate<uint16_t>> dilate_u16{"dilate_u16"};
