#include <iostream>
#include <climits>
#include "Halide.h"

using namespace Halide;

class Histogram : public Halide::Generator<Histogram> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", std::numeric_limits<uint8_t>::max() + 1};
    ImageParam src{UInt(8), 2, "src"};

    Var x;

    Func build() {

        Func dst("dst");
        dst(x) = cast<uint32_t>(0);
        Expr hist_size = std::numeric_limits<uint8_t>::max() + 1;
        Func bin_size;
        bin_size() = (hist_size + hist_width - 1) / hist_width;
        bin_size.compute_root();
        RDom r(0, width, 0, height);
        Expr idx = cast<int32_t>(src(r.x, r.y) / bin_size());
        dst(idx) += cast<uint32_t>(1);

        return dst;
    }
};

RegisterGenerator<Histogram> histogram{"histogram"};
