#include <cstdint>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T, typename D>
class Sq_sum : public Halide::Generator<Sq_sum<T, D>> {
public:

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst{"dst"};

        dst = Element::sq_sum<T, D>(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {1, 1});

        return dst;
    }
};

using Sq_sum_u8_f32 = Sq_sum<uint8_t, float>;
HALIDE_REGISTER_GENERATOR(Sq_sum_u8_f32 , sq_sum_u8_f32);
using Sq_sum_u16_f32 = Sq_sum<uint16_t, float>;
HALIDE_REGISTER_GENERATOR(Sq_sum_u16_f32 , sq_sum_u16_f32);
using Sq_sum_u32_f32 = Sq_sum<uint32_t, float>;
HALIDE_REGISTER_GENERATOR(Sq_sum_u32_f32 , sq_sum_u32_f32);
using Sq_sum_u8_f64 = Sq_sum<uint8_t, double>;
HALIDE_REGISTER_GENERATOR(Sq_sum_u8_f64 , sq_sum_u8_f64);
using Sq_sum_u16_f64 = Sq_sum<uint16_t, double>;
HALIDE_REGISTER_GENERATOR(Sq_sum_u16_f64 , sq_sum_u16_f64);
using Sq_sum_u32_f64 = Sq_sum<uint32_t, double>;
HALIDE_REGISTER_GENERATOR(Sq_sum_u32_f64 , sq_sum_u32_f64);
