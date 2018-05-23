#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class ThresholdTozeroInv : public Halide::Generator<ThresholdTozeroInv<T>> {

    ImageParam src{type_of<T>(), 2, "src"};
    Param<T> threshold{"threshold", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
public:
    Func build() {
        Func dst{"dst"};
        dst = Element::threshold_tozero_inv<T>(src, threshold);

        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(ThresholdTozeroInv<uint8_t>, threshold_tozero_inv_u8);
HALIDE_REGISTER_GENERATOR(ThresholdTozeroInv<uint16_t>, threshold_tozero_inv_u16);
