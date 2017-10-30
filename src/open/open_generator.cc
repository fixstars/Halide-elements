#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Open : public Halide::Generator<Open<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};
    
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};

    Var x, y;

    // Generalized Func from erode/dilate
    Func conv_with_structure(Func src_img, std::function<Expr(RDom, Expr)> f, Expr init) {
        Func input("input");
        input(x, y) = src_img(x, y);
        schedule(input, {width, height});

        Func structure_("structure_");
        structure_(x, y) = structure(x, y);
        schedule(structure_, {window_width, window_height});

        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        Func allzero("allzero");
        Var tmp("tmp");
        allzero(tmp) = cast<bool>(true);
        allzero(tmp) = allzero(tmp) && (structure_(r.x + window_width / 2, r.y + window_height / 2) == 0);
        allzero.compute_root();

        schedule(allzero, {1});
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            Expr val = select(allzero(0), clamped(x - window_width / 2, y - window_height / 2),
                              f(r, select(structure_(r.x + window_width / 2, r.y + window_height / 2) == 0, init, clamped(x + r.x, y + r.y))));
            workbuf(x, y) = val;
            workbuf.compute_root();
            input = workbuf;
            schedule(workbuf, {width, height});
        }

        return input;
    }

    Func build() {
        Func erode = conv_with_structure(src, [](RDom r, Expr e){return minimum_unroll(r, e);}, type_of<T>().max());
        Func dilate = conv_with_structure(erode, [](RDom r, Expr e){return maximum_unroll(r, e);}, type_of<T>().min());

        schedule(src, {width, height});
        schedule(structure, {window_width, window_height});
        
        return dilate;
    }
};

RegisterGenerator<Open<uint8_t>> open_u8{"open_u8"};
RegisterGenerator<Open<uint16_t>> open_u16{"open_u16"};
