#include <iostream>
#include "Halide.h"
#include "Element.h"
#include "Image.h"

using namespace Halide;

template<typename T>
class Gaussian : public Halide::Generator<Gaussian<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};
    Param<double> sigma{"sigma", 1.0};

    Func build() {
        return Element::gaussian<T>(src, width, height, window_width, window_height, sigma);
    }
};

RegisterGenerator<Gaussian<uint8_t>> gaussian_u8{"gaussian_u8"};
RegisterGenerator<Gaussian<uint16_t>> gaussian_u16{"gaussian_u16"};
