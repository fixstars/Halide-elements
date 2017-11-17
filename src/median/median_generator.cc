#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Median : public Halide::Generator<Median<T>> {
    Var x, y;
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};

    Func build() {
        Func dst = median(src, width, height, window_width, window_height);
        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Median<uint8_t>> median_u8{"median_u8"};
RegisterGenerator<Median<uint16_t>> median_u16{"median_u16"};
