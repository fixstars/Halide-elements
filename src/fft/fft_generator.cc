#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

class FFT : public Halide::Generator<FFT> {
    GeneratorParam<int32_t> n_{"n", 256};
    GeneratorParam<int32_t> batch_size_{"batch_size", 4};
    ImageParam in{Float(32), 3, "in"};

public:
    Func build()
    {
        Var c{"c"}, i{"i"}, k{"k"};
        Func out("out");
        const int32_t n = static_cast<uint32_t>(n_);
        const int32_t batch_size = static_cast<uint32_t>(batch_size_);

        out(c, i, k) = Element::fft(in, n, batch_size)(c, i, k);

        schedule(in, {2, n, batch_size});
        schedule(out, {2, n, batch_size}).unroll(c);

        return out;
    }
};

HALIDE_REGISTER_GENERATOR(FFT, fft);
