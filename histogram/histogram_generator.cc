#include <iostream>
#include <climits>
#include "Halide.h"

using namespace Halide;

class Histogram : public Halide::Generator<Histogram> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", UCHAR_MAX + 1};
    ImageParam src{UInt(8), 2, "src"};

    Var x;

    Func build() {

        Func dst("dst");
	dst(x) = cast<uint32_t>(0);
	RDom r(0, width, 0, height);
	Expr idx = cast<int32_t>(src(r.x, r.y) * cast<int32_t>(hist_width - 1) / cast<int32_t>(UCHAR_MAX));
	dst(idx) += cast<uint32_t>(1);
        
        return dst;
    }
};

RegisterGenerator<Histogram> histogram{"histogram"};
