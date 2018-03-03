#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Or : public Halide::Generator<Or<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::filter_or<T>(src0, src1);
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Or<uint8_t>> or_u8{"or_u8"};
RegisterGenerator<Or<uint16_t>> or_u16{"or_u16"};
RegisterGenerator<Or<uint32_t>> or_u32{"or_u32"};
