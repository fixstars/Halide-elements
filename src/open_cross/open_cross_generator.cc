#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class OpenCross : public Halide::Generator<OpenCross<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    Param<int32_t> window_width{"window_width", 3, 3, 17};
    Param<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

    // Generalized Func from erode/dilate
    Func gen_erode(Func src_img, std::function<Expr(Expr)> f) {
        
        Func input("input");
        input(x, y) = src_img(x, y);

        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        r.where(r.x == 0 || r.y == 0);
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            Expr val = f(clamped(x + r.x, y + r.y));
            workbuf(x, y) = val;
            workbuf.compute_root();
            input = workbuf;
        }

        return input;
    }

    Func build() {
        auto mn = std::bind(static_cast<Expr(*)(Expr, const std::string&)>(Halide::minimum), std::placeholders::_1, "minimum");
        Func erode = gen_erode(src, mn);
        auto mx = std::bind(static_cast<Expr(*)(Expr, const std::string&)>(Halide::maximum), std::placeholders::_1, "maximum");
        Func dilate = gen_erode(erode, mx);
        return dilate;
    }
};

RegisterGenerator<OpenCross<uint8_t>> open_cross_u8{"open_cross_u8"};
RegisterGenerator<OpenCross<uint16_t>> open_cross_u16{"open_cross_u16"};
