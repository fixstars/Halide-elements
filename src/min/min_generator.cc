#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Min_ : public Halide::Generator<Min_<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    Func build() {
        Func dst = Element::min(src0, src1);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Min_<uint8_t>> min_u8{"min_u8"};
RegisterGenerator<Min_<uint16_t>> min_u16{"min_u16"};
RegisterGenerator<Min_<uint32_t>> min_u32{"min_u32"};
