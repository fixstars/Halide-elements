#include <cstdint>

#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Multiply : public Halide::Generator<Multiply<T>> {
public:
    ImageParam src1{type_of<T>(), 2, "src1"};
    ImageParam src2{type_of<T>(), 2, "src2"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::multiply<T>(src1, src2);

        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Multiply<uint8_t>> multiply_u8{"multiply_u8"};
RegisterGenerator<Multiply<uint16_t>> multiply_u16{"multiply_u16"};
RegisterGenerator<Multiply<uint32_t>> multiply_u32{"multiply_u32"};
