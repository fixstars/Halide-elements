#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Bilateral : public Halide::Generator<Bilateral<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<int32_t> window_size{"window_size", 1};
    Param<double> sigma_color{"sigma_color", 1};
    Param<double> sigma_space{"sigma_space", 1};


    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::bilateral<T>(src, width, height, window_size, sigma_color, sigma_space);

        window_size.set_range(1, 7);
        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Bilateral<uint8_t>, bilateral_u8);
HALIDE_REGISTER_GENERATOR(Bilateral<uint16_t>, bilateral_u16);
