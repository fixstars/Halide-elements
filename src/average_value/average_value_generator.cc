#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename S, typename D>
class Average_value : public Halide::Generator<Average_value<S, D>>{
public:
    ImageParam src{type_of<S>(), 2, "src"};
    ImageParam roi{type_of<uint8_t>(), 2, "roi"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build(){
        Func dst{"dst"};
        dst = Element::average_value<D>(src, roi, width, height);
        schedule(src, {width, height});
        schedule(roi, {width, height});
        schedule(dst, {1});

        return dst;
    }
};
RegisterGenerator<Average_value<uint8_t, float>> average_value_u8_f32{"average_value_u8_f32"};
RegisterGenerator<Average_value<uint16_t, float>> average_value_u16_f32{"average_value_u16_f32"};

RegisterGenerator<Average_value<uint8_t, double>> average_value_u8_f64{"average_value_u8_f64"};
RegisterGenerator<Average_value<uint16_t, double>> average_value_u16_f64{"average_value_u16_f64"};
