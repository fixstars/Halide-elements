#include <iostream>
#include <climits>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Histogram : public Halide::Generator<Histogram<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> hist_width{"hist_width", std::numeric_limits<T>::max() + 1};
    ImageParam src{type_of<T>(), 2, "src"};

    Var x;

    Func build() {

        Func dst("dst");
        dst(x) = cast<uint32_t>(0);
        Expr hist_size = cast<uint32_t>(type_of<T>().max()) + 1;
        Func bin_size;
        bin_size() = (hist_size + hist_width - 1) / hist_width;
        bin_size.compute_root();
        RDom r(0, width, 0, height);
        Expr idx = cast<int32_t>(src(r.x, r.y) / bin_size());
        dst(idx) += cast<uint32_t>(1);

        return dst;
    }
};

RegisterGenerator<Histogram<uint8_t>> histogram_u8{"histogram_u8"};
RegisterGenerator<Histogram<uint16_t>> histogram_u16{"histogram_u16"};
