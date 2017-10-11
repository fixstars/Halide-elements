#include <iostream>
#include <climits>
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
	Expr dstval = cast<uint8_t>(min(srcval / value, cast<float>(UCHAR_MAX)) + 1.0f) & ~0x1;
        dst(x, y) = dstval;
        
        return dst;
    }
};

RegisterGenerator<DivScalar> div_scalar{"div_scalar"};
