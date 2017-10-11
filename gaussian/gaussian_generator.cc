#include <iostream>
#include "Halide.h"

using namespace Halide;

class Gaussian : public Halide::Generator<Gaussian> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{UInt(8), 2, "src"};
    Param<uint32_t> window_width{"window_width"};
    Param<uint32_t> window_height{"window_height"};
    Param<float> sigma{"sigma"};

    Var x, y;

    Func build() {

	Func clamped = BoundaryConditions::repeat_edge(src);
	RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
	Func kernel("kernel");
	kernel(x, y) = 0.0f;
	kernel(r.x, r.y) = exp(-(r.x * r.x + r.y * r.y) / (2 * sigma * sigma));
	kernel.compute_root();
	Func kernel_sum("kernel_sum");
	kernel_sum() = 0.0f;
	kernel_sum() += kernel(r.x, r.y);
	kernel_sum.compute_root();
        Func dst("dst");
	Expr dstval = sum(clamped(x + r.x, y + r.y) * kernel(r.x, r.y));
	dst(x,y) = cast<uint8_t>(dstval / kernel_sum() + 0.5f);
        
        return dst;
    }
};

RegisterGenerator<Gaussian> gaussian{"gaussian"};
