#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;


class SecondPass : public Halide::Generator<SecondPass> {
public:
    ImageParam src{type_of<uint32_t>(), 2, "src"};
    ImageParam buf{type_of<uint32_t>(), 2, "buf"};
    Param<int32_t> bufW{"bufW", 1};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func dst{"dst"};
        dst = Element::label_secondpass(src, buf, width, height, bufW);

        schedule(src, {width, height});
        schedule(dst, {width, height});
        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(SecondPass, second_pass);
