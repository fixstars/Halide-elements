#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Merge3 : public Halide::Generator<Merge3<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};
    ImageParam src2{type_of<T>(), 2, "src2"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::merge3(src0, src1, src2, width, height);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(dst, {3, width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Merge3<uint8_t>, merge3_u8);
HALIDE_REGISTER_GENERATOR(Merge3<uint16_t>, merge3_u16);
HALIDE_REGISTER_GENERATOR(Merge3<int8_t>, merge3_i8);
HALIDE_REGISTER_GENERATOR(Merge3<int16_t>, merge3_i16);
