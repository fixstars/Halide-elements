#include <iostream>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Open : public Halide::Generator<Open<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};
    Param<int32_t> window_width{"window_width", 3, 3, 17};
    Param<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

    Func erode(Func src) {

        Func input("input");
        input(x, y) = src(x, y);

        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        Func allzero("allzero");
        allzero() = cast<bool>(true);
        allzero() = allzero() && (structure(r.x + window_width / 2, r.y + window_height / 2) == 0);
        allzero.compute_root();
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            Expr val = select(allzero(), clamped(x - window_width / 2, y - window_height / 2), minimum(select(structure(r.x + window_width / 2, r.y + window_height / 2) == 0, type_of<T>().max(), clamped(x + r.x, y + r.y))));
            workbuf(x, y) = val;
            workbuf.compute_root();
            input = workbuf;
        }

        return input;
    }

    Func dilate(Func src) {

        Func input("input");
        input(x, y) = src(x, y);

        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        Func allzero("allzero");
        allzero() = cast<bool>(true);
        allzero() = allzero() && (structure(r.x + window_width / 2, r.y + window_height / 2) == 0);
        allzero.compute_root();
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            Expr val = select(allzero(), clamped(x - window_width / 2, y - window_height / 2), maximum(select(structure(r.x + window_width / 2, r.y + window_height / 2) == 0, type_of<T>().min(), clamped(x + r.x, y + r.y))));
            workbuf(x, y) = val;
            workbuf.compute_root();
            input = workbuf;
        }

        return input;
    }

    Func build() {
        return dilate(erode(src));
    }
};

RegisterGenerator<Open<uint8_t>> open_u8{"open_u8"};
RegisterGenerator<Open<uint16_t>> open_u16{"open_u16"};
