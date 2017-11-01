#include <iostream>
#include <cmath>
#include "Halide.h"
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class POC : public Generator<POC> {
public:
    GeneratorParam<int32_t> n_{"n", 16};
    GeneratorParam<int32_t> batch_size_{"batch_size", 16};
    ImageParam input1{Float(32), 2, "input1"};
    ImageParam input2{Float(32), 2, "input2"};

    Var c{"c"}, i{"i"}, k{"k"}, h{"h"};
    Var x{"x"}, y{"y"};

    Func fft(Func in) {
        const int32_t n = static_cast<int32_t>(n_);
        const int32_t batch_size = static_cast<int32_t>(batch_size_);


        Func weight("weight");
        Expr theta = static_cast<float>(-2.0 * M_PI) * cast<float>(i) / static_cast<float>(n);
        weight(c, i) = select(c == 0, cos(theta), sin(theta));

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

    Func fft2(Func cmp1) {
        const int32_t n = static_cast<int32_t>(n_);

        Func fftx("fftx");
        fftx = fft(cmp1);

        Func tr("tr");
        tr(c, x, y) = fftx(c, y, x);

        Func ffty("ffty");
        ffty = fft(tr);

        return ffty;
    }

    Func ifft2(Func a) {
        const int32_t n = static_cast<int32_t>(n_);

        Func a_conj, fft_a_conj, fft_a_conj2, ifft_a;

        a_conj      = complex_conj(a);
        fft_a_conj  = fft2(a_conj);
        fft_a_conj2 = complex_conj(fft_a_conj);

        ifft_a(c, x, y) = fft_a_conj2(c, x, y) / (n * n);

        return ifft_a;
    }

    Func complex_conj(Func a) {
        Func ret;
        ret(c, i, k) = 0.0f;
        ret(0, i, k) = a(0, i, k);
        ret(1, i, k) = -a(1, i, k);

        return ret;
    }

    Func complex_mul(Func a, Func b) {
        Func ret("ret1");
        ret(c, i, k) = 0.0f;
        ret(0, i, k) = a(0, i, k) * b(0, i, k) - a(1, i, k) * b(1, i, k);
        ret(1, i, k) = a(0, i, k) * b(1, i, k) + a(1, i, k) * b(0, i, k);

        return ret;
    }

    Func complex_normal(Func a) {
        Func len, ret;
        len(i, k) = sqrt(a(0, i, k) * a(0, i, k) + a(1, i, k) * a(1, i, k));
        ret(c, i, k) = a(c, i, k) / len(i, k);

        return ret;
    }

    Func build() {
        const int32_t n = static_cast<int32_t>(n_);

        Func w, hann_w_tmp, hann_w;
        w(x, y) = sqrt((x / (float)n - 0.5f) * (x / (float)n - 0.5f) + (y / (float)n - 0.5f) * (y / (float)n - 0.5f));
        hann_w_tmp(x, y) = 0.5f - 0.5f * cos(2.0f * (float)M_PI * ((w(x, y) + 0.5f)));
        hann_w(x, y) = select(w(x, y) <= 0.5f, hann_w_tmp(x, y), 0.0f);

        Func hann_w_in1, hann_w_in2;
        hann_w_in1(c, x, y) = select(c==0, hann_w(x, y) * input1(x, y), 0);
        hann_w_in2(c, x, y) = select(c==0, hann_w(x, y) * input2(x, y), 0);

        Func fft_in1, fft_in2;
        fft_in1 = fft2(hann_w_in1);
        fft_in2 = fft2(hann_w_in2);

        Func r, r_normal;
        r = complex_mul(fft_in2, complex_conj(fft_in1));
        r_normal = complex_normal(r);

        Func ifft_r_n;
        ifft_r_n = ifft2(r_normal);

        Func func_poc("func_poc");
        func_poc(x, y) = ifft_r_n(0, x, y);

        return func_poc;
    }

private:
    Expr E(int32_t v)
    {
        return make_const(UInt(32), static_cast<uint32_t>(v));
    }
};

RegisterGenerator<POC> poc{"poc"};

