#include <iostream>
#include <cmath>
#include "Halide.h"
#include "Element.h"
#include "Image.h"

using namespace Halide;

class Affine : public Generator<Affine> {
public:
    GeneratorParam<int32_t> width{"width", 768};
    GeneratorParam<int32_t> height{"height", 1280};
    ImageParam input{UInt(8), 2, "input"};
    Param<float> degrees{"degrees", 0.0f};
    Param<float> scale_x{"scale_x", 1.0f};
    Param<float> scale_y{"scale_y", 1.0f};
    Param<float> shift_x{"shift_x", 0.0f};
    Param<float> shift_y{"shift_y", 0.0f};
    Param<float> skew_y{"skew_y", 0.0f};


    Func build() {
        return Element::affine(input, width, height, degrees, scale_x, scale_y, shift_x, shift_y, skew_y);
    }
};

RegisterGenerator<Affine> affine{"affine"};

