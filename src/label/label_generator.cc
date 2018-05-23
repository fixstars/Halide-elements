#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Label : public Halide::Generator<Label<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
         dst = Element::label_firstpass(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Label<uint8_t>, label_u8);
HALIDE_REGISTER_GENERATOR(Label<uint16_t>, label_u16);
