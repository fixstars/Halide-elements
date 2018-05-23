#include <cstdint>
#include "Halide.h"

#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

class SinCos : public Halide::Generator<SinCos> {
    ImageParam src{type_of<float>(), 2, "src"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

public:
    Func build() {
        Func dst{"dst"};
        Var x, y;
        FixedN<30, 28, true> e =
            select((x + y) % 2 == 0,
                   Element::sin(to_fixed<30, 25>(src(x, y))),
                   Element::cos(to_fixed<30, 25>(src(x, y))));
        dst(x, y) = from_fixed<float>(e);

        schedule(src, {width, height});
        schedule(dst, {width, height});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(SinCos, sin_cos);
