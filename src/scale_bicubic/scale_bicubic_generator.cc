#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class ScaleBicubic : public Halide::Generator<ScaleBicubic<T>> {
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> in_width{"in_width", 1024};
    GeneratorParam<int32_t> in_height{"in_height", 768};

    GeneratorParam<int32_t> out_width{"out_width", 500};
    GeneratorParam<int32_t> out_height{"out_height", 500};

public:
    Func build() {
        Func dst{"dst"};
        dst = Element::scale_bicubic<T>(src, in_width, in_height,out_width, out_height);

        schedule(src, {in_width, in_height});
        schedule(dst, {out_width, out_height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(ScaleBicubic<uint8_t>, scale_bicubic_u8);
HALIDE_REGISTER_GENERATOR(ScaleBicubic<uint16_t>, scale_bicubic_u16);
HALIDE_REGISTER_GENERATOR(ScaleBicubic<int16_t>, scale_bicubic_i16);
