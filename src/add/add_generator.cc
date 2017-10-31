#include <iostream>
#include "Halide.h"

using namespace Halide;

class Add : public Halide::Generator<Add> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{UInt(8), 2, "src0"}, src1{UInt(8), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
        Expr srcval0 = cast<uint16_t>(src0(x, y)), srcval1 = cast<uint16_t>(src1(x, y));
	
        Expr dstval = cast<uint8_t>(min(srcval0 + srcval1, cast<uint16_t>(255)));
        dst(x, y) = dstval;

        return dst;
    }
};

RegisterGenerator<Add> add{"add"};
