#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T, typename D>
class Integral : public Halide::Generator<Integral<T, D>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::integral<D>(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

using Integral_u8_f32 = Integral<uint8_t, float>;
HALIDE_REGISTER_GENERATOR(Integral_u8_f32 , integral_u8_f32);
using Integral_u16_f32 = Integral<uint16_t, float>;
HALIDE_REGISTER_GENERATOR(Integral_u16_f32 , integral_u16_f32);
using Integral_u32_f32 = Integral<uint32_t, float>;
HALIDE_REGISTER_GENERATOR(Integral_u32_f32 , integral_u32_f32);
using Integral_u8_f64 = Integral<uint8_t, double>;
HALIDE_REGISTER_GENERATOR(Integral_u8_f64 , integral_u8_f64);
using Integral_u16_f64 = Integral<uint16_t, double>;
HALIDE_REGISTER_GENERATOR(Integral_u16_f64 , integral_u16_f64);
using Integral_u32_f64 = Integral<uint32_t, double>;
HALIDE_REGISTER_GENERATOR(Integral_u32_f64 , integral_u32_f64);
