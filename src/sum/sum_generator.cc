#include <cstdint>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Sum : public Halide::Generator<Sum<T>> {
public:

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst{"dst"};

        dst =  Element::sum<T>(src);

        schedule(src, {width, height});
        schedule(dst, {1, 1});

        return dst;
    }
};

RegisterGenerator<Sum<uint8_t>> sum_u8{"sum_u8"};
RegisterGenerator<Sum<uint16_t>> sum_u16{"sum_u16"};
RegisterGenerator<Sum<uint32_t>> sum_u32{"sum_u32"};
