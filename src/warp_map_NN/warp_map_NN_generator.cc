#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class WarpMapNN : public Halide::Generator<WarpMapNN<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<float>(), 2, "src1"};
    ImageParam src2{type_of<float>(), 2, "src2"};
    GeneratorParam<int32_t> border_type{"border_type", 0}; //0 or 1
    Param<T> border_value{"border_value", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::warp_map_NN<T>(src0, src1, src2, border_type, border_value, width, height);
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(WarpMapNN<uint8_t>, warp_map_NN_u8);
HALIDE_REGISTER_GENERATOR(WarpMapNN<uint16_t>, warp_map_NN_u16);
