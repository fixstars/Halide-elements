#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Max_ : public Halide::Generator<Max_<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    Func build() {
        Func dst = Element::max(src0, src1);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Max_<uint8_t>> max_u8{"max_u8"};
RegisterGenerator<Max_<uint16_t>> max_u16{"max_u16"};
RegisterGenerator<Max_<uint32_t>> max_u32{"max_u32"};
