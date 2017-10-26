#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Gaussian : public Halide::Generator<Gaussian<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};
    Param<float> sigma{"sigma", 1.0};

    Func build() {
        Var x{"x"}, y{"y"};
        
        Func clamped = BoundaryConditions::repeat_edge(src);
        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        Func kernel("kernel");
        kernel(x, y) = exp(-(x * x + y * y) / (2 * sigma * sigma));
        
        Func kernel_sum("kernel_sum");
        kernel_sum(x) = sum(kernel(r.x, r.y));
        kernel_sum.compute_root();
        Func dst("dst");
        Expr dstval = cast<float>(sum(clamped(x + r.x, y + r.y) * kernel(r.x, r.y)));
        dst(x,y) = cast<T>(round(dstval / kernel_sum(0)));

        schedule(src, {width, height});
        kernel.compute_root();
        kernel.bound(x, -(window_width / 2), window_width);
        kernel.bound(y, -(window_height / 2), window_height);
        schedule(kernel_sum, {1});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<Gaussian<uint8_t>> gaussian_u8{"gaussian_u8"};
RegisterGenerator<Gaussian<uint16_t>> gaussian_u16{"gaussian_u16"};
