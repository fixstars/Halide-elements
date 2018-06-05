#include <cstdint>

#include <Halide.h>
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

class Convolution : public Halide::Generator<Convolution> {
    ImageParam in{UInt(8), 2, "in"};
    ImageParam kernel{Int(16), 2, "kernel"};
    Param<int32_t> kernel_size{"kernel_size", 3, 1, 5};

    GeneratorParam<int32_t> width{"width", 512};
    GeneratorParam<int32_t> height{"height", 512};
    GeneratorParam<int32_t> unroll_factor{"unroll_factor", 2};

public:
    Func build() {
        Func out{"out"};

        out = Element::convolution<16, 10>(in, width, height, kernel, kernel_size.get(), unroll_factor);

        schedule(in, {width, height});
        schedule(kernel, {5, 5});
        schedule(out, {width, height});

        if (unroll_factor) {
            out.unroll(out.args()[0], unroll_factor);
        }

        return out;
    }
};

HALIDE_REGISTER_GENERATOR(Convolution, convolution)
