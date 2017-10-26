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
    Param<float> value{"value", 1};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval = src(x, y);
        Expr dstval = min(srcval / value, cast<float>(type_of<T>().max()));
        dstval = max(dstval, 0);
        dst(x, y) = cast<T>(round(dstval));

        schedule(src, {width, height});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<DivScalar<uint8_t>> div_scalar_u8{"div_scalar_u8"};
RegisterGenerator<DivScalar<uint16_t>> div_scalar_u16{"div_scalar_u16"};
RegisterGenerator<DivScalar<uint32_t>> div_scalar_u32{"div_scalar_u32"};
