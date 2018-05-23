#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Laplacian : public Halide::Generator<Laplacian<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 8};
    GeneratorParam<int32_t> height{"height", 8};

    Func build() {
        Func dst{"dst"};
        dst = Element::laplacian<T>(src, width, height);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Laplacian<uint8_t>, laplacian_u8);
HALIDE_REGISTER_GENERATOR(Laplacian<uint16_t>, laplacian_u16);
