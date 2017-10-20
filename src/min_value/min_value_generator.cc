#include <iostream>
#include <typeinfo>
#include "Halide.h"

using namespace Halide;

template<typename T>
class Min_value : public Halide::Generator<Min_value<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam roi{type_of<uint8_t>(), 2, "roi"};

    Var x{"x"}, y{"y"};
    
    Func build() {
        Func ones("ones"), count("count"), dst("dst");        
        Var d("d");

        ones(x, y) = 1;
        RDom r(0, width, 0, height, "r");
        r.where(roi(r.x, r.y) != 0);
        count(d) = cast<int32_t>(0);
        count(0) = sum(ones(r.x, r.y));
        
        dst(d) = cast<T>(0);
        dst(0) = cast<T>(select(count(0) == 0, 0, minimum(src(r.x, r.y))));
        return dst;
    }
};

RegisterGenerator<Min_value<uint8_t>> min_value_u8{"min_value_u8"};
RegisterGenerator<Min_value<uint16_t>> min_value_u16{"min_value_u16"};
RegisterGenerator<Min_value<uint32_t>> min_value_u32{"min_value_u32"};
