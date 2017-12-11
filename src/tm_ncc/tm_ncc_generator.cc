#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class TmNcc : public Halide::Generator<TmNcc<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> img_width{"img_width", 1024};
    GeneratorParam<int32_t> img_height{"img_height", 768};
    GeneratorParam<int32_t> tmp_width{"tmp_width", 16};
    GeneratorParam<int32_t> tmp_height{"tmp_height", 16};

    Func build() {
        Func dst{"dst"};

        dst = Element::tm_ncc<T>(src0, src1, img_width, img_height, tmp_width, tmp_height);

        return dst;
    }
};

RegisterGenerator<TmNcc<uint8_t>> tm_ncc_u8{"tm_ncc_u8"};
RegisterGenerator<TmNcc<uint16_t>> tm_ncc_u16{"tm_ncc_u16"};
RegisterGenerator<TmNcc<uint32_t>> tm_ncc_u32{"tm_ncc_u32"};
