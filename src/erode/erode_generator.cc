#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

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
        Var x{"x"}, y{"y"};
        Func dst("dst");

        dst(x, y) = Element::erode<T>(src, width, height, window_width, window_height, structure, iteration)(x, y);

        schedule(src, {width, height});
        schedule(structure, {window_width, window_height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Erode<uint8_t>, erode_u8);
HALIDE_REGISTER_GENERATOR(Erode<uint16_t>, erode_u16);
