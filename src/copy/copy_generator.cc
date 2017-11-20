#include <iostream>
#include "Halide.h"
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Copy : public Halide::Generator<Copy<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{type_of<T>(), 2, "src"};

    Var x, y;

    Func build() {
        return copy<T>(src);
    }
};

RegisterGenerator<Copy<uint8_t>> copy_u8{"copy_u8"};
RegisterGenerator<Copy<uint16_t>> copy_u16{"copy_u16"};
