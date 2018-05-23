#include <cstdint>
#include "Halide.h"
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Copy : public Halide::Generator<Copy<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::copy(src);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Copy<uint8_t>, copy_u8);
HALIDE_REGISTER_GENERATOR(Copy<uint16_t>, copy_u16);
