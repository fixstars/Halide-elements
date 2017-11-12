#include <Halide.h>
#include <Element.h>
#include "ImageProcessing.h"

using namespace Halide;

class SimpleISP : public Halide::Generator<SimpleISP> {
    GeneratorParam<int32_t> width{"width", 3280};
    GeneratorParam<int32_t> height{"height", 2486};
    ImageParam in{UInt(16), 2, "in"};
    Param<uint16_t> optical_black_value{"optical_black_value", 0};
    Param<float> gamma_value{"gamma_value", 1.0f};
    Param<float> saturation_value{"saturation_value", 1.0f};

public:
    Func build() {
        return Element::simple_isp(in, optical_black_value, gamma_value, saturation_value, width, height);
    }

};

HALIDE_REGISTER_GENERATOR(SimpleISP, "simple_isp")
