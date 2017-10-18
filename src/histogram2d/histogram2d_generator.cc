#include <iostream>
#include <climits>
#include <cassert>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Histogram2D : public Halide::Generator<Histogram2D<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", 256, 1, static_cast<int32_t>(sqrt(std::numeric_limits<int32_t>::max()))};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Var x, y;

    Func build() {

        Func dst("dst");
        dst(x, y) = cast<uint32_t>(0);
        RDom r(0, width, 0, height);
        Expr idx0 = cast<int32_t>(src0(r.x, r.y) * cast<uint64_t>(hist_width) / (cast<uint64_t>(type_of<T>().max()) + 1));
        Expr idx1 = cast<int32_t>(src1(r.x, r.y) * cast<uint64_t>(hist_width) / (cast<uint64_t>(type_of<T>().max()) + 1));
        dst(idx0, idx1) += cast<uint32_t>(1);

        return dst;
    }
};

RegisterGenerator<Histogram2D<uint8_t>> histogram2d_u8{"histogram2d_u8"};
RegisterGenerator<Histogram2D<uint16_t>> histogram2d_u16{"histogram2d_u16"};
