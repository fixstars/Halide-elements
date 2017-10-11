#include <iostream>
#include <climits>
#include "Halide.h"

using namespace Halide;

class Histogram2D : public Halide::Generator<Histogram2D> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", UCHAR_MAX + 1};
    ImageParam src0{UInt(8), 2, "src0"}, src1{UInt(8), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
	dst(x, y) = cast<uint32_t>(0);
	RDom r(0, width, 0, height);
	Expr idx0 = cast<int32_t>(src0(r.x, r.y) * cast<int32_t>(hist_width - 1) / cast<int32_t>(UCHAR_MAX));
	Expr idx1 = cast<int32_t>(src1(r.x, r.y) * cast<int32_t>(hist_width - 1) / cast<int32_t>(UCHAR_MAX));
	dst(idx0, idx1) += cast<uint32_t>(1);
        
        return dst;
    }
};

RegisterGenerator<Histogram2D> histogram2d{"histogram2d"};
