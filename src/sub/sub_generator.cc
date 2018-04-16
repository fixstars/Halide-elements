#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Sub : public Halide::Generator<Sub<T>> {
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

public:
    Func build() {
        Func dst{"dst"};
        dst =  Element::sub<T>(src0, src1);
        
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Sub<uint8_t>> sub_u8{"sub_u8"};
RegisterGenerator<Sub<uint16_t>> sub_u16{"sub_u16"};
RegisterGenerator<Sub<uint32_t>> sub_u32{"sub_u32"};
