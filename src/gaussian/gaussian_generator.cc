#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Gaussian : public Halide::Generator<Gaussian<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<int32_t> width{"width", 1024};
    Param<int32_t> height{"height", 768};
    Param<int32_t> window_width{"window_width", 3};
    Param<int32_t> window_height{"window_height", 3};
    Param<float> sigma{"sigma", 1.0};

    Var x, y;

    Func build() {

        Func clamped = BoundaryConditions::repeat_edge(src);
        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        Func kernel("kernel");
        kernel(x, y) = 0.0f;
        kernel(r.x, r.y) = exp(-(r.x * r.x + r.y * r.y) / (2 * sigma * sigma));
        kernel.compute_root();
        Func kernel_sum("kernel_sum");
        kernel_sum() = sum(kernel(r.x, r.y));
        kernel_sum.compute_root();
        Func dst("dst");
        Expr dstval = cast<float>(sum(clamped(x + r.x, y + r.y) * kernel(r.x, r.y)));
        dst(x,y) = cast<T>(round(dstval / kernel_sum()));

        return dst;
    }
};

RegisterGenerator<Gaussian<uint8_t>> gaussian_u8{"gaussian_u8"};
RegisterGenerator<Gaussian<uint16_t>> gaussian_u16{"gaussian_u16"};
