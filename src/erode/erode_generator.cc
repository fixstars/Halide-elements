#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Erode : public Halide::Generator<Erode<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam structure{UInt(8), 2, "structure"};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

    Var x, y;

    Func build() {

        Func input("input");
        input(x, y) = src(x, y);

        Func structure_("structure_");
        structure_(x, y) = structure(x, y);
        schedule(structure, {window_width, window_height});
        schedule(structure_, {window_width, window_height});
        
        RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
        Func allzero("allzero");
        allzero(x) = cast<bool>(true);
        allzero(x) = allzero(x) && (structure_(r.x + window_width / 2, r.y + window_height / 2) == 0);
        schedule(allzero, {1});
        for (int32_t i = 0; i < iteration; i++) {
            Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
            Func workbuf("workbuf");
            Expr val = select(allzero(0), clamped(x - window_width / 2, y - window_height / 2), minimum_unroll(r, select(structure_(r.x + window_width / 2, r.y + window_height / 2) == 0, type_of<T>().max(), clamped(x + r.x, y + r.y))));
            workbuf(x, y) = val;
            schedule(workbuf, {width, height});
            input = workbuf;
        }

        schedule(src, {width, height});
        return input;
    }
};

RegisterGenerator<Erode<uint8_t>> erode_u8{"erode_u8"};
RegisterGenerator<Erode<uint16_t>> erode_u16{"erode_u16"};
