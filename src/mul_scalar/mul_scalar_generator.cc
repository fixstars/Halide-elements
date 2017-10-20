#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class MulScalar : public Halide::Generator<MulScalar<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    Param<float> value{"value", 1};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval = src(x, y);
        Expr dstval = min(srcval * value, cast<float>(type_of<T>().max()));
        dstval = max(dstval, 0);
        dst(x, y) = cast<T>(round(dstval));

        return dst;
    }
};

RegisterGenerator<MulScalar<uint8_t>> mul_scalar_u8{"mul_scalar_u8"};
RegisterGenerator<MulScalar<uint16_t>> mul_scalar_u16{"mul_scalar_u16"};
RegisterGenerator<MulScalar<uint32_t>> mul_scalar_u32{"mul_scalar_u32"};
