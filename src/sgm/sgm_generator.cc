#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class SGM : public Halide::Generator<SGM> {
public:
    ImageParam in_l{UInt(8), 2, "in_l"};
    ImageParam in_r{UInt(8), 2, "in_r"};

    GeneratorParam<int32_t> disp{"disp", 16};
    GeneratorParam<int32_t> width{"width", 641};
    GeneratorParam<int32_t> height{"height", 555};

    Func build()
    {
        Func out{"out"};

        out = semi_global_matching(in_l, in_r, width, height, disp);

        schedule(in_l, {width, height});
        schedule(in_r, {width, height});
        schedule(out, {width, height});

        return out;
    }

private:

};
HALIDE_REGISTER_GENERATOR(SGM, sgm)
