#include <iostream>
#include "Halide.h"
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class Copy : public Halide::Generator<Copy> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src{UInt(8), 2, "src"};

    Var x, y;

    Func build() {
        Func dst("dst");
        dst(x, y) = src(x, y);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

RegisterGenerator<Copy> copy{"copy"};
