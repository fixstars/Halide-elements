#include <iostream>
#include "Halide.h"
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Copy : public Halide::Generator<Copy<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<int32_t> width{"width", 1024};
    Param<int32_t> height{"height", 768};

    Var x, y;

    Func build() {
        Func dst("dst");
        dst(x, y) = src(x, y);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Copy<uint8_t>> copy_u8{"copy_u8"};
RegisterGenerator<Copy<uint16_t>> copy_u16{"copy_u16"};
