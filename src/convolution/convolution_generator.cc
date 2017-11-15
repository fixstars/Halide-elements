#include <Halide.h>
#include <Element.h>
#include "ImageProcessing.h"

using namespace Halide;
   
class Convolution : public Halide::Generator<Convolution> {
    GeneratorParam<int32_t> width{"width", 512};
    GeneratorParam<int32_t> height{"height", 512};
    GeneratorParam<int32_t> unroll_factor{"unroll_factor", 2};
    ImageParam in{UInt(8), 2, "in"};
    ImageParam kernel{Int(16), 2, "kernel"};
    Param<int32_t> kernel_size{"kernel_size", 3, 1, 5};
       
public:
    Func build() {
        return Element::convolution(in, width, height, kernel, kernel_size.get(), unroll_factor);
    }
};

HALIDE_REGISTER_GENERATOR(Convolution, "convolution")
