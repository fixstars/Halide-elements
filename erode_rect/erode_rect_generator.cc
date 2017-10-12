#include <iostream>
#include "Halide.h"

using namespace Halide;

class ErodeRect : public Halide::Generator<ErodeRect> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{UInt(8), 2, "src"};
    Param<int32_t> window_width{"window_width"};
    Param<int32_t> window_height{"window_height"};

    Var x, y;

    Func build() {

        window_width.set_range(3, 17);
        window_height.set_range(3, 17);

        Func input("input");
        input(x, y) = src(x, y);
        
        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            Expr val = minimum(clamped(x + r.x, y + r.y));
            workbuf(x, y) = val;
            workbuf.compute_root();
            input = workbuf;
        }
        
        return input;
    }
};

RegisterGenerator<ErodeRect> erode_rect{"erode_rect"};
