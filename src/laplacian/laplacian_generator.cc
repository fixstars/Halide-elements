#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Laplacian : public Halide::Generator<Laplacian<T>> {
public:
    GeneratorParam<int32_t> width{"width", 8};
    GeneratorParam<int32_t> height{"height", 8};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Var x{"x"}, y{"y"};
        
        Func clamped = BoundaryConditions::repeat_edge(src);
        RDom r(-1, 3, -1, 3);
        Func kernel("kernel");
        kernel(x, y) = cast<double>(-1);
        kernel(0, 0) = cast<double>(8);

        Func dst("dst");
        Expr dstval = sum(cast<double>(clamped(x + r.x, y + r.y)) * kernel(r.x, r.y));
        dstval = select(dstval < 0, -dstval, dstval);
        dstval = select(dstval > type_of<T>().max(), cast<double>(type_of<T>().max()), dstval);
        dst(x, y) = cast<T>(dstval);

        kernel.compute_root();
        kernel.compute_root();
        kernel.bound(x, -1, 3);
        kernel.bound(y, -1, 3);
        schedule(src, {width, height});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<Laplacian<uint8_t>> laplacian_u8{"laplacian_u8"};
RegisterGenerator<Laplacian<uint16_t>> laplacian_u16{"laplacian_u16"};
