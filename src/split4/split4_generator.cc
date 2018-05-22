#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Split4 : public Halide::Generator<Split4<T>> {
public:
    ImageParam src{type_of<T>(), 3, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst;
        dst = Element::split4(src, width, height);

        schedule(src, {4, width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Split4<uint8_t>, split4_u8);
HALIDE_REGISTER_GENERATOR(Split4<uint16_t>, split4_u16);
HALIDE_REGISTER_GENERATOR(Split4<int8_t>, split4_i8);
HALIDE_REGISTER_GENERATOR(Split4<int16_t>, split4_i16);
