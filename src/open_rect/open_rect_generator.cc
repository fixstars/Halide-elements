#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class OpenRect : public Halide::Generator<OpenRect<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

    // Generalized Func from erode/dilate
    Func conv_rect(Func src_img, std::function<Expr(RDom, Expr)> f) {
        Func input("input");
        input(x, y) = src_img(x, y);
        schedule(input, {width, height});

        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            schedule(workbuf, {width, height}); // <- this line cause SEGV!
            Expr val = f(r, clamped(x + r.x, y + r.y));
            workbuf(x, y) = val;
            workbuf.compute_root();
            input = workbuf;
            schedule(workbuf, {width, height});
        }

        return input;
    }

    Func build() {
        Func erode = conv_rect(src, [](RDom r, Expr e){return minimum_unroll(r, e);});
        Func dilate = conv_rect(erode, [](RDom r, Expr e){return maximum_unroll(r, e);});

        schedule(src, {width, height});
        
        return dilate;
    }
};

RegisterGenerator<OpenRect<uint8_t>> open_rect_u8{"open_rect_u8"};
RegisterGenerator<OpenRect<uint16_t>> open_rect_u16{"open_rect_u16"};
