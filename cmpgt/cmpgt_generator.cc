#include <iostream>
#include <climits>
#include "Halide.h"

using namespace Halide;

class Cmpgt : public Halide::Generator<Cmpgt> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{UInt(8), 2, "src0"}, src1{UInt(8), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
	Expr srcval0 = src0(x, y), srcval1 = src1(x, y);
	Expr dstval = cast<uint8_t>(srcval0 > srcval1);
        dst(x, y) = dstval * UCHAR_MAX;
        
        return dst;
    }
};

RegisterGenerator<Cmpgt> cmpgt{"cmpgt"};
