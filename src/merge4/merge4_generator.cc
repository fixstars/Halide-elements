#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Merge4 : public Halide::Generator<Merge4<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};
    ImageParam src2{type_of<T>(), 2, "src2"};
    ImageParam src3{type_of<T>(), 2, "src3"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::merge4(src0, src1, src2, src3, width, height);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(src3, {width, height});
        schedule(dst, {4, width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Merge4<uint8_t>, merge4_u8);
HALIDE_REGISTER_GENERATOR(Merge4<uint16_t>, merge4_u16);
HALIDE_REGISTER_GENERATOR(Merge4<int8_t>, merge4_i8);
HALIDE_REGISTER_GENERATOR(Merge4<int16_t>, merge4_i16);
