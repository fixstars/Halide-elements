#include <Halide.h>
#include <Element.h>

using namespace Halide;

class FFT : public Halide::Generator<FFT> {
    GeneratorParam<int32_t> n_{"n", 256};
    GeneratorParam<int32_t> batch_size_{"batch_size", 4};
    ImageParam in{Float(32), 3, "in"};

public:
    Func build()
    {
        const int32_t n = static_cast<uint32_t>(n_);
        const int32_t batch_size = static_cast<uint32_t>(batch_size_);
        return Element::fft(in, n, batch_size);
    }

private:
};

RegisterGenerator<FFT> fft{"fft"};

