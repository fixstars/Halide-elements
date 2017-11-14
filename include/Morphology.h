#pragma once

#include <vector>
#include <Halide.h>

#include "Schedule.h"
#include "Reduction.h"

namespace Halide {
namespace Element {

template<typename T>
Halide::Func dilate(Halide::Func input, int32_t width, int32_t height, int32_t window_width, int32_t window_height, Halide::Func structure_, int32_t iteration)
{
	using namespace Halide;
	using namespace Halide::Element;

	Var x, y;

	RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
	Func allzero("allzero");
	allzero(x) = cast<bool>(true);
	allzero(x) = allzero(x) && (structure_(r.x + window_width / 2, r.y + window_height / 2) == 0);
	schedule(allzero, {1});

	for (int32_t i = 0; i < iteration; i++) {
		Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
		Func workbuf("workbuf");
		Expr val = select(allzero(0), clamped(x - window_width / 2, y - window_height / 2), maximum_unroll(r, select(structure_(r.x + window_width / 2, r.y + window_height / 2) == 0, type_of<T>().min(), clamped(x + r.x, y + r.y))));
		workbuf(x, y) = val;
		schedule(workbuf, {width, height});
		input = workbuf;
	}

	schedule(input, {width, height});
	return input;
}

Func conv_rect(Func src_img, std::function<Expr(RDom, Expr)> f, int32_t width, int32_t height, int32_t iteration, int32_t window_width, int32_t window_height) {
    Var x, y;

    Func input("input");
    input(x, y) = src_img(x, y);
    schedule(input, {width, height});

    RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
    for (int32_t i = 0; i < iteration; i++) {
        Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
        Func workbuf("workbuf");
        Expr val = f(r, clamped(x + r.x, y + r.y));
        workbuf(x, y) = val;
        workbuf.compute_root();
        input = workbuf;
        schedule(workbuf, {width, height});
    }

    return input;
}


Func conv_cross(Func src_img, std::function<Expr(RDom, Expr)> f, int32_t width, int32_t height, int32_t iteration, int32_t window_width, int32_t window_height) {
    Var x, y;

    Func input("input");
    input(x, y) = src_img(x, y);
    schedule(input, {width, height});

    RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
    r.where(r.x == 0 || r.y == 0);
    for (int32_t i = 0; i < iteration; i++) {
        Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
        Func workbuf("workbuf");
        Expr val = f(r, clamped(x + r.x, y + r.y));
        workbuf(x, y) = val;
        workbuf.compute_root();
        schedule(workbuf, {width, height});
        input = workbuf;
    }

    return input;
}


Func conv_with_structure(Func src_img, std::function<Expr(RDom, Expr)> f, Expr init, Func structure, int32_t width, int32_t height, int32_t window_width, int32_t window_height, int32_t iteration) {
    Var x, y;

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

} // Element
} // Halide

