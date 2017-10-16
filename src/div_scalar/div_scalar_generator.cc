#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class DivScalar : public Halide::Generator<DivScalar<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<int32_t> width{"width", 1024};
    Param<int32_t> height{"height", 768};
    Param<float> value{"value", 1};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval = src(x, y);
        Expr dstval = min(srcval / value, cast<float>(type_of<T>().max()));
        dstval = max(dstval, 0);
        dst(x, y) = cast<T>(round(dstval));

        return dst;
    }
};

RegisterGenerator<DivScalar<uint8_t>> div_scalar_u8{"div_scalar_u8"};
RegisterGenerator<DivScalar<uint16_t>> div_scalar_u16{"div_scalar_u16"};
RegisterGenerator<DivScalar<uint32_t>> div_scalar_u32{"div_scalar_u32"};
