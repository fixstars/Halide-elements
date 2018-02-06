#include <cstdint>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Sq_sum : public Halide::Generator<Sq_sum<T>> {
public:

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst{"dst"};

        dst =  Element::sq_sum<T>(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {1, 1});

        return dst;
    }
};

RegisterGenerator<Sq_sum<uint8_t>> sq_sum_u8{"sq_sum_u8"};
RegisterGenerator<Sq_sum<uint16_t>> sq_sum_u16{"sq_sum_u16"};
RegisterGenerator<Sq_sum<uint32_t>> sq_sum_u32{"sq_sum_u32"};
