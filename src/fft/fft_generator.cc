#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class FFT : public Halide::Generator<FFT> {
    Var c{"c"}, i{"i"}, k{"k"};

    GeneratorParam<int32_t> n_{"n", 256};
    GeneratorParam<int32_t> batch_size_{"batch_size", 4};
    ImageParam in{Float(32), 3, "in"};

public:
    Func build()
    {
        const int32_t n = static_cast<int32_t>(n_);
        const int32_t batch_size = static_cast<int32_t>(batch_size_);

        Func weight("weight");
        Expr theta = static_cast<float>(-2.0 * M_PI) * cast<float>(i) / static_cast<float>(n);
        weight(c, i) = select(c ==0, cos(theta), sin(theta));
        
        Func stage("in");
        stage(c, i, k) = in(c, i, k);

        for (int j=0; j<log2(n); ++j) {

            stage = BoundaryConditions::repeat_edge(stage, {{0, 2}, {0, n}, {0, batch_size}});

            Func next_stage("stage" + std::to_string(j));

            const int m = (n >> (j + 1));

            Expr cond = (i % (n >> j)) < m;

            Expr o = select(cond, i + m, i - m);

            ComplexExpr vi = {stage(0, i, k), stage(1, i, k)};
            ComplexExpr vo = {stage(0, o, k), stage(1, o, k)};

            // Case 1
            ComplexExpr v1 = vi + vo;

            // Case 2
            Expr wi = (i % m) * (1<<j);
            ComplexExpr w = {weight(0, wi), weight(1, wi)};
            ComplexExpr v2 = (vo - vi) * w;
            next_stage(c, i, k) = select(cond, select(c == 0, v1.x, v1.y),
                                               select(c == 0, v2.x, v2.y));

            schedule(next_stage, {2, n, batch_size}).unroll(c);

            stage = next_stage;
        }

        // Make bit-reversal 32-bit integer index
        Expr ri = cast<uint32_t>(i);
        ri = (ri & E(0x55555555)) <<  1 | (ri & E(0xAAAAAAAA)) >>  1;
        ri = (ri & E(0x33333333)) <<  2 | (ri & E(0xCCCCCCCC)) >>  2;
        ri = (ri & E(0x0F0F0F0F)) <<  4 | (ri & E(0xF0F0F0F0)) >>  4;
        ri = (ri & E(0x00FF00FF)) <<  8 | (ri & E(0xFF00FF00)) >>  8;
        ri = (ri & E(0x0000FFFF)) << 16 | (ri & E(0xFFFF0000)) >> 16;
        ri = cast<int32_t>(ri >> (32 - log2(n)));

        stage = BoundaryConditions::repeat_edge(stage, {{0, 2}, {0, n}, {0, batch_size}});

        Func out("out");
        out(c, i, k) = stage(c, ri, k);

        schedule(in, {2, n, batch_size});
        schedule(weight, {2, n/2});
        schedule(out, {2, n, batch_size}).unroll(c);
        
        return out;
    }

private:
    Expr E(int32_t v)
    {
        return make_const(UInt(32), static_cast<uint32_t>(v));
    }
};

HALIDE_REGISTER_GENERATOR(FFT, "fft")
