#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;
   
class Convolution : public Halide::Generator<Convolution> {
    Var x{"x"}, y{"y"};

    GeneratorParam<int32_t> width{"width", 512};
    GeneratorParam<int32_t> height{"height", 512};
    GeneratorParam<int32_t> unroll_factor{"unroll_factor", 2};
    ImageParam in{UInt(8), 2, "in"};
    ImageParam kernel{Int(16), 2, "kernel"};
    Param<int32_t> kernel_size{"kernel_size", 3, 1, 5};
       
public:
    Func build() {
        Func bounded = BoundaryConditions::repeat_edge(in, 0, width.value(), 0, height.value());

        Expr kh = div_round_to_zero(kernel_size, 2);
        RDom r(0, kernel_size, 0, kernel_size);

        Expr dx = r.x - kh;
        Expr dy = r.y - kh;

        Func k;
        k(x, y) = kernel(x, y);
        
        constexpr uint32_t frac_bits = 10;
        using Fixed16 = Fixed<int16_t, frac_bits>;
        Fixed16 pv = to_fixed<int16_t, frac_bits>(bounded(x+dx, y+dy));
        Fixed16 kv{k(r.x, r.y)};

        Func out("out");
        out(x, y) = from_fixed<uint8_t>(sum_unroll(r, pv * kv));

        schedule(in, {width, height});
        schedule(kernel, {5, 5});
        schedule(k, {5, 5});
        schedule(out, {width, height}).unroll(x, unroll_factor);
      
        return out;
    }
};

HALIDE_REGISTER_GENERATOR(Convolution, "convolution")
