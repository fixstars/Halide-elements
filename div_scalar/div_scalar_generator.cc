#include <iostream>
#include "Halide.h"

using namespace Halide;

class DivScalar : public Halide::Generator<DivScalar> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{UInt(8), 2, "src"};
    Param<float> value{"value", 1};
  
    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval = src(x, y);
        Expr dstval = min(srcval / value, cast<float>(type_of<uint8_t>().max()));
        dstval = max(dstval, 0);
        dst(x, y) = cast<uint8_t>(round(dstval));
        
        return dst;
    }
};

RegisterGenerator<DivScalar> div_scalar{"div_scalar"};
