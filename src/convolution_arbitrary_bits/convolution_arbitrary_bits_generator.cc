#include <cstdint>

#include <Halide.h>
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

class Convolution : public Halide::Generator<Convolution> {
    static constexpr uint32_t NB = 20;
    static constexpr uint32_t FB = 10;
    static constexpr uint32_t UB = 32;

    ImageParam in{UInt(8), 2, "in"};
#if defined(HALIDE_FOR_FPGA)
    ImageParam kernel{Int(NB), 2, "kernel"};
#else
    ImageParam kernel{Int(UB), 2, "kernel"};
#endif
    Param<int32_t> kernel_size{"kernel_size", 3, 1, 5};

    GeneratorParam<int32_t> width{"width", 512};
    GeneratorParam<int32_t> height{"height", 512};
    GeneratorParam<int32_t> unroll_factor{"unroll_factor", 2};

public:
    Func build() {
        Func out{"out"};

        out = Element::convolution<NB, FB>(in, width, height, kernel, kernel_size.get(), unroll_factor);

        schedule(in, {width, height});
        schedule(kernel, {5, 5});
        schedule(out, {width, height});

        if (unroll_factor) {
            out.unroll(out.args()[0], unroll_factor);
        }

        return out;
    }
};

HALIDE_REGISTER_GENERATOR(Convolution, convolution_arbitrary_bits)
