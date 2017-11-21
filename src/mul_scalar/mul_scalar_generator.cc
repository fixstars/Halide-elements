#include <iostream>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class MulScalar : public Halide::Generator<MulScalar<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    Param<float> value{"value", 1};

    Var x, y;

    Func build() {
        return mul_scalar<T>(src, value);
    }
};

RegisterGenerator<MulScalar<uint8_t>> mul_scalar_u8{"mul_scalar_u8"};
RegisterGenerator<MulScalar<uint16_t>> mul_scalar_u16{"mul_scalar_u16"};
RegisterGenerator<MulScalar<uint32_t>> mul_scalar_u32{"mul_scalar_u32"};
