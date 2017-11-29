#include <cstdint>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class MinValue : public Halide::Generator<MinValue<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    ImageParam roi{type_of<uint8_t>(), 2, "roi"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};

        dst = Element::min_value<T>(src, roi, width, height);

        schedule(src, {width, height});
        schedule(roi, {width, height});
        schedule(dst, {1});

        return dst;
    }
};

RegisterGenerator<MinValue<uint8_t>> min_value_u8{"min_value_u8"};
RegisterGenerator<MinValue<uint16_t>> min_value_u16{"min_value_u16"};
RegisterGenerator<MinValue<uint32_t>> min_value_u32{"min_value_u32"};
