#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Split3 : public Halide::Generator<Split3<T>> {
public:
    ImageParam src{type_of<T>(), 3, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst;
        dst = Element::split3(src, width, height);

        schedule(src, {3, width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Split3<uint8_t>, split3_u8);
HALIDE_REGISTER_GENERATOR(Split3<uint16_t>, split3_u16);
HALIDE_REGISTER_GENERATOR(Split3<int8_t>, split3_i8);
HALIDE_REGISTER_GENERATOR(Split3<int16_t>, split3_i16);
