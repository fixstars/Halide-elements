#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Sad : public Halide::Generator<Sad<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    //GeneratorParam<int32_t> width{"width", 10};
    //GeneratorParam<int32_t> height{"height", 8};

    Func build() {
        Func dst{"dst"};

        dst = Element::sad<T>(src0, src1, width, height);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Sad<uint8_t>> sad_u8{"sad_u8"};
RegisterGenerator<Sad<uint16_t>> sad_u16{"sad_u16"};
RegisterGenerator<Sad<uint32_t>> sad_u32{"sad_u32"};
