#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class SetScalar : public Halide::Generator<SetScalar<T>> {
    Param<T> value{"value", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

public:
    Func build() {
        Func dst{"dst"};
        dst = Element::set_scalar(value);
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<SetScalar<uint8_t>> set_scalar_u8{"set_scalar_u8"};
RegisterGenerator<SetScalar<uint16_t>> set_scalar_u16{"set_scalar_u16"};
