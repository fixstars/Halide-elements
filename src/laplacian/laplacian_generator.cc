#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Laplacian : public Halide::Generator<Laplacian<T>> {
public:
    GeneratorParam<int32_t> width{"width", 8};
    GeneratorParam<int32_t> height{"height", 8};
    ImageParam src{type_of<T>(), 2, "src"};

    Func build() {
        Func dst = laplacian<T>(src, width, height);
        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Laplacian<uint8_t>> laplacian_u8{"laplacian_u8"};
RegisterGenerator<Laplacian<uint16_t>> laplacian_u16{"laplacian_u16"};
