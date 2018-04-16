#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T, typename D>
class SqIntegral : public Halide::Generator<SqIntegral<T, D>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::sq_integral<D>(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<SqIntegral<uint8_t, float>> sq_integral_u8_f32{"sq_integral_u8_f32"};
RegisterGenerator<SqIntegral<uint16_t, float>> sq_integral_u16_f32{"sq_integral_u16_f32"};
RegisterGenerator<SqIntegral<uint32_t, float>> sq_integral_u32_f32{"sq_integral_u32_f32"};
RegisterGenerator<SqIntegral<uint8_t, double>> sq_integral_u8_f64{"sq_integral_u8_f64"};
RegisterGenerator<SqIntegral<uint16_t, double>> sq_integral_u16_f64{"sq_integral_u16_f64"};
RegisterGenerator<SqIntegral<uint32_t, double>> sq_integral_u32_f64{"sq_integral_u32_f64"};
