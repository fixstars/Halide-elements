#include <iostream>
#include <typeinfo>
#include "Halide.h"

using namespace Halide;

template<typename T>
class MinValue : public Halide::Generator<MinValue<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam roi{type_of<uint8_t>(), 2, "roi"};

    Func build() {
        Func count("count"), dst("dst");

        RDom r(0, width, 0, height, "r");
        r.where(roi(r.x, r.y) != 0);
        count() = sum(select(roi(r.x, r.y) == 0, 0, 1));
        
        dst() = cast<T>(select(count() == 0, 0, minimum(src(r.x, r.y))));
        return dst;
    }
};

RegisterGenerator<MinValue<uint8_t>> min_value_u8{"min_value_u8"};
RegisterGenerator<MinValue<uint16_t>> min_value_u16{"min_value_u16"};
RegisterGenerator<MinValue<uint32_t>> min_value_u32{"min_value_u32"};
