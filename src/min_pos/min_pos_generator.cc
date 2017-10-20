#include <iostream>
#include <typeinfo>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Min_pos : public Halide::Generator<Min_pos<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Var x{"x"}, y{"y"};
    
    Func build() {
        Func dst("dst");
        RDom r(0, width, 0, height, "r");
        Tuple res = argmin(src(r.x, r.y));

        Var d("d");
        dst(d) = cast<uint32_t>(0);
        dst(0) = cast<uint32_t>(res[0]);
        dst(1) = cast<uint32_t>(res[1]);
        return dst;
    }
};

RegisterGenerator<Min_pos<uint8_t>> min_pos_u8{"min_pos_u8"};
RegisterGenerator<Min_pos<uint16_t>> min_pos_u16{"min_pos_u16"};
RegisterGenerator<Min_pos<uint32_t>> min_pos_u32{"min_pos_u32"};
RegisterGenerator<Min_pos<float>> min_pos_f32{"min_pos_f32"};
RegisterGenerator<Min_pos<double>> min_pos_f64{"min_pos_f64"};
