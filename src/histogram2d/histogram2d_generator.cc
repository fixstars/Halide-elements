#include <climits>
#include <cmath>
#include <cstdint>

#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Histogram2D : public Halide::Generator<Histogram2D<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", 256, 1, static_cast<int32_t>(sqrt(std::numeric_limits<int32_t>::max()))};

    Func build() {
        Func dst{"dst"};

        dst = Element::histogram2d<T>(src0, src1, width, height, hist_width);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {hist_width, hist_width});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Histogram2D<uint8_t>, histogram2d_u8);
HALIDE_REGISTER_GENERATOR(Histogram2D<uint16_t>, histogram2d_u16);
