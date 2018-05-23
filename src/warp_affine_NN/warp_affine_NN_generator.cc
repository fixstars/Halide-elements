#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class WarpAffineNN : public Halide::Generator<WarpAffineNN<T>> {
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> border_type{"border_type", 1}; //0 or 1
    Param<T> border_value{"border_value", 1};
    ImageParam transform{type_of<double>(), 1, "transform"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};


public:
    Func build() {
        Func dst{"dst"};
        dst = Element::warp_affine_NN<T>(src, border_type, border_value, transform, width, height);
        schedule(src, {width, height});
        schedule(transform, {6});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(WarpAffineNN<uint8_t>, warp_affine_NN_u8);
HALIDE_REGISTER_GENERATOR(WarpAffineNN<uint16_t>, warp_affine_NN_u16);
