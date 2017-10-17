#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class FloatErr : public Halide::Generator<FloatErr<T>> {
public:
    ImageParam src{type_of<T>(), 1, "src"};
    Param<int32_t> width{"width", 1};
    ImageParam filter{Float(32), 1, "filter"};
    Param<int32_t> window_width{"window_width", 2};

    Var x;

    Func build() {

        RDom r(0, window_width);
        Func dst("dst");
        Expr dstval = sum(src(x + r.x) * filter(r.x));
        dst(x) = cast<float>(dstval);

        return dst;
    }
};

RegisterGenerator<FloatErr<uint8_t>> float_err_u8{"float_err_u8"};
