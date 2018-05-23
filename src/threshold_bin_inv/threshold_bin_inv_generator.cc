#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class ThresholdBinInv : public Halide::Generator<ThresholdBinInv<T>> {

    ImageParam src{type_of<T>(), 2, "src"};
    Param<T> threshold{"threshold", 1};
    Param<T> value{"value", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
public:
    Func build() {
        Func dst{"dst"};
        dst = Element::threshold_binary_inv<T>(src, threshold, value);

        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(ThresholdBinInv<uint8_t>, threshold_bin_inv_u8);
HALIDE_REGISTER_GENERATOR(ThresholdBinInv<uint16_t>, threshold_bin_inv_u16);
