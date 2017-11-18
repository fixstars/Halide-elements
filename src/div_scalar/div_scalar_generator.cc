#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class DivScalar : public Halide::Generator<DivScalar<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    Param<double> value{"value", 2.0};

    Var x, y;

    Func build() {
        return div_scalar<T>(src, value);
    }
};

RegisterGenerator<DivScalar<uint8_t>> div_scalar_u8{"div_scalar_u8"};
RegisterGenerator<DivScalar<uint16_t>> div_scalar_u16{"div_scalar_u16"};
RegisterGenerator<DivScalar<uint32_t>> div_scalar_u32{"div_scalar_u32"};
