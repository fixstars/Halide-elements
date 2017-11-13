#include <Halide.h>
#include <Element.h>
#include <Sgm.h>

using namespace Halide;
using namespace Halide::Element;

class SGM : public Halide::Generator<SGM> {
    Var x{"x"}, y{"y"}, d{"d"};

    GeneratorParam<int32_t> disp{"disp", 16};
    GeneratorParam<int32_t> width{"width", 641};
    GeneratorParam<int32_t> height{"height", 555};
    
    ImageParam in_l{UInt(8), 2, "in_l"};
    ImageParam in_r{UInt(8), 2, "in_r"};

public:
    Func build()
    {
        return sgm(in_l, in_r, disp, width, height);
    }

private:

};
HALIDE_REGISTER_GENERATOR(SGM, "sgm")
