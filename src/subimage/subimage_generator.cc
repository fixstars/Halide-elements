#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Subimage : public Halide::Generator<Subimage<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> in_width{"in_width", 1024};
    GeneratorParam<int32_t> in_height{"in_height", 768};

    GeneratorParam<int32_t> out_width{"out_width", 500};
    GeneratorParam<int32_t> out_height{"out_height", 500};

    Param<uint32_t> origin_x{"origin_x", 1};
    Param<uint32_t> origin_y{"origin_y", 1};

    Func build() {
        Func dst{"dst"};
        dst = Element::subimage<T>(src, origin_x, origin_y);
        schedule(src, {in_width, in_height});
        schedule(dst, {out_width, out_height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Subimage<uint8_t>, subimage_u8);
HALIDE_REGISTER_GENERATOR(Subimage<uint16_t>, subimage_u16);
HALIDE_REGISTER_GENERATOR(Subimage<uint32_t>, subimage_u32);
