#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Xor : public Halide::Generator<Xor<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::filter_xor(src0, src1);
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Xor<uint8_t>> xor_u8{"xor_u8"};
RegisterGenerator<Xor<uint16_t>> xor_u16{"xor_u16"};
RegisterGenerator<Xor<uint32_t>> xor_u32{"xor_u32"};
