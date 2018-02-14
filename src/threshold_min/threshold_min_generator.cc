#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class ThresholdMin : public Halide::Generator<ThresholdMin<T>> {

    ImageParam src{type_of<T>(), 2, "src"};
    Param<T> threshold{"threshold", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
public:
    Func build() {
        Func dst{"dst"};
        dst = Element::threshold_min<T>(src, threshold);

        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

RegisterGenerator<ThresholdMin<uint8_t>> threshold_min_u8{"threshold_min_u8"};
RegisterGenerator<ThresholdMin<uint16_t>> threshold_min_u16{"threshold_min_u16"};
