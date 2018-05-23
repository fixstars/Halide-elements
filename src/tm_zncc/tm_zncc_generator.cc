#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class TmZncc : public Halide::Generator<TmZncc<T>> {
public:
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};

    GeneratorParam<int32_t> img_width{"img_width", 1024};
    GeneratorParam<int32_t> img_height{"img_height", 768};
    GeneratorParam<int32_t> tmp_width{"tmp_width", 16};
    GeneratorParam<int32_t> tmp_height{"tmp_height", 16};

    Func build() {
        Func dst{"dst"};

        dst = Element::tm_zncc<T>(src0, src1, img_width, img_height, tmp_width, tmp_height);

        const int32_t res_width = img_width.value() - tmp_width.value() + 1;
        const int32_t res_height = img_height.value() - tmp_height.value() + 1;

        schedule(src0, {img_width, img_height});
        schedule(src1, {tmp_width, tmp_height});
        schedule(dst, {res_width, res_height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(TmZncc<uint8_t>, tm_zncc_u8);
HALIDE_REGISTER_GENERATOR(TmZncc<uint16_t>, tm_zncc_u16);
HALIDE_REGISTER_GENERATOR(TmZncc<uint32_t>, tm_zncc_u32);
