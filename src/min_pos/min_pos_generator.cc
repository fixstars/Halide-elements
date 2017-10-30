#include <iostream>
#include <typeinfo>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class MinPos : public Halide::Generator<MinPos<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst("dst");
        RDom r(0, width, 0, height, "r");
        Func res("res");
        Var x("x");
        res(x) = argmin(r, src(r.x, r.y));
        
        Var d("d");
        dst(d) = cast<uint32_t>(0);
        dst(0) = cast<uint32_t>(res(0)[0]);
        dst(1) = cast<uint32_t>(res(0)[1]);

        schedule(src, {width, height});
        schedule(dst, {2});
        schedule(res, {1});

        return dst;
    }
};

RegisterGenerator<MinPos<uint8_t>> min_pos_u8{"min_pos_u8"};
RegisterGenerator<MinPos<uint16_t>> min_pos_u16{"min_pos_u16"};
RegisterGenerator<MinPos<uint32_t>> min_pos_u32{"min_pos_u32"};
RegisterGenerator<MinPos<float>> min_pos_f32{"min_pos_f32"};
RegisterGenerator<MinPos<double>> min_pos_f64{"min_pos_f64"};
