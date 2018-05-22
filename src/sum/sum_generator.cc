#include <cstdint>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T, typename D>
class Sum : public Halide::Generator<Sum<T, D>> {
public:

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst{"dst"};

        dst = Element::sum<T, D>(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {1, 1});

        return dst;
    }
};

using Sum_u8_f32 = Sum<uint8_t, float>;
HALIDE_REGISTER_GENERATOR(Sum_u8_f32, sum_u8_f32);
using Sum_u16_f32 = Sum<uint16_t, float>;
HALIDE_REGISTER_GENERATOR(Sum_u16_f32, sum_u16_f32);
using Sum_u32_f32 = Sum<uint32_t, float>;
HALIDE_REGISTER_GENERATOR(Sum_u32_f32, sum_u32_f32);
using Sum_u8_f64 = Sum<uint8_t, double>;
HALIDE_REGISTER_GENERATOR(Sum_u8_f64, sum_u8_f64);
using Sum_u16_f64 = Sum<uint16_t, double>;
HALIDE_REGISTER_GENERATOR(Sum_u16_f64, sum_u16_f64);
using Sum_u32_f64 = Sum<uint32_t, double>;
HALIDE_REGISTER_GENERATOR(Sum_u32_f64, sum_u32_f64);
using Sum_f32_f32 = Sum<float, float>;
HALIDE_REGISTER_GENERATOR(Sum_f32_f32, sum_f32_f32);
using Sum_f64_f64 = Sum<double, double>;
HALIDE_REGISTER_GENERATOR(Sum_f64_f64, sum_f64_f64);
