#include <iostream>
#include <typeinfo>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class MaxPos : public Halide::Generator<MaxPos<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst = max_pos(src, width, height);
        schedule(src, {width, height});
        schedule(dst, {2});

        return dst;
    }
};

RegisterGenerator<MaxPos<uint8_t>> max_pos_u8{"max_pos_u8"};
RegisterGenerator<MaxPos<uint16_t>> max_pos_u16{"max_pos_u16"};
RegisterGenerator<MaxPos<uint32_t>> max_pos_u32{"max_pos_u32"};
RegisterGenerator<MaxPos<float>> max_pos_f32{"max_pos_f32"};
RegisterGenerator<MaxPos<double>> max_pos_f64{"max_pos_f64"};
