#include <climits>
#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Histogram : public Halide::Generator<Histogram<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", std::numeric_limits<T>::max() + 1};

    Func build() {
        Func dst{"dst"};

        dst = Element::histogram<T>(src, width, height, hist_width);

        schedule(src, {width, height});
        schedule(dst, {hist_width});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Histogram<uint8_t>, histogram_u8);
HALIDE_REGISTER_GENERATOR(Histogram<uint16_t>, histogram_u16);
