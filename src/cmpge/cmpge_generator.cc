#include <cstdint>

#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Cmpge : public Halide::Generator<Cmpge<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::cmpge<T>(src0, src1);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Cmpge<uint8_t>, cmpge_u8);
HALIDE_REGISTER_GENERATOR(Cmpge<uint16_t>, cmpge_u16);
HALIDE_REGISTER_GENERATOR(Cmpge<uint32_t>, cmpge_u32);
