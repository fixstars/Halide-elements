#include <iostream>
#include "Halide.h"

using namespace Halide;

class Erode : public Halide::Generator<Erode> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<uint32_t> iteration{"iteration", 2};
    ImageParam src{UInt(8), 2, "src"};
    ImageParam structure{UInt(8), 1, "structure"};
    Param<uint32_t> window_width{"window_width"};
    Param<uint32_t> window_height{"window_height"};

    Var x, y;

    Func build() {
	
	Func input("input");
	input(x, y) = src(x, y);

	RDom r(-(window_width / 2), window_width, -(window_height / 2), window_height);
	Func invkval("invkval");
	invkval(x, y) = cast<uint8_t>(0);
	invkval(r.x, r.y) = ~structure(window_width / 2 + r.x + window_width * (window_height / 2 + r.y));
	invkval.compute_root();
	for (int i = 0; i < iteration; i++) {
	    Func clamped = BoundaryConditions::repeat_edge(input, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});
	    Func workbuf("workbuf");
	    Expr val = cast<uint8_t>(minimum(cast<uint32_t>(clamped(x + r.x, y + r.y)) + cast<uint32_t>(invkval(r.x, r.y)) + cast<uint32_t>(1) - cast<uint32_t>(cast<uint8_t>(invkval(r.x, r.y)) + cast<uint8_t>(1))));
	    workbuf(x, y) = val;
	    workbuf.compute_root();
	    input = workbuf;
	}

	return input;
    }
};

RegisterGenerator<Erode> erode{"erode"};
