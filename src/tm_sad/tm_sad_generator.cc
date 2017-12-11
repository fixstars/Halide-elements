#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class TmSad : public Halide::Generator<TmSad<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> img_width{"img_width", 1024};
    GeneratorParam<int32_t> img_height{"img_height", 768};
    GeneratorParam<int32_t> tmp_width{"tmp_width", 16};
    GeneratorParam<int32_t> tmp_height{"tmp_height", 16};

    Func build() {
        Func dst{"dst"};

        dst = Element::tm_sad<T>(src0, src1, img_width, img_height, tmp_width, tmp_height);

        return dst;
    }
};

RegisterGenerator<TmSad<uint8_t>> tm_sad_u8{"tm_sad_u8"};
RegisterGenerator<TmSad<uint16_t>> tm_sad_u16{"tm_sad_u16"};
RegisterGenerator<TmSad<uint32_t>> tm_sad_u32{"tm_sad_u32"};
