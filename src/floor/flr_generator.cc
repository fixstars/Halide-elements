#include <cstdint>
#include "Halide.h"
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;
using Halide::Element::schedule;

class Floor : public Halide::Generator<Floor> {
public:
    ImageParam src{type_of<float>(), 1, "src"};

    GeneratorParam<int32_t> width{"width", 21};

    using base_t = int32_t;
    static constexpr uint32_t NB = 32;
    static constexpr uint32_t FB = 12;
    using fixed_t = FixedN<NB, FB, true>;
    
    Func build() {
        Func dst{"dst"};
        Var x;
        fixed_t val;
        
        val = fixed_t::to_fixed(src(x));
        dst(x) = fixed_t::from_fixed<float>(floor(val));

        schedule(src, {width});
        schedule(dst, {width});

        return dst;
    }
};

HALIDE_REGISTER_GENERATOR(Floor, flr);
