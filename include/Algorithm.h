#ifndef HALIDE_ELEMENT_ALGORITHM_H
#define HALIDE_ELEMENT_ALGORITHM_H

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include <Halide.h>
#include "Schedule.h"
#include "Complex.h"

namespace Halide {
namespace Element {

namespace {
Expr ConstU32(int32_t v)
{
    return Halide::Internal::make_const(UInt(32), static_cast<uint32_t>(v));
}

Expr bit_reverse(Expr i, const int n)
{
    Expr ri = cast<uint32_t>(i);
    ri = (ri & ConstU32(0x55555555)) <<  1 | (ri & ConstU32(0xAAAAAAAA)) >>  1;
    ri = (ri & ConstU32(0x33333333)) <<  2 | (ri & ConstU32(0xCCCCCCCC)) >>  2;
    ri = (ri & ConstU32(0x0F0F0F0F)) <<  4 | (ri & ConstU32(0xF0F0F0F0)) >>  4;
    ri = (ri & ConstU32(0x00FF00FF)) <<  8 | (ri & ConstU32(0xFF00FF00)) >>  8;
    ri = (ri & ConstU32(0x0000FFFF)) << 16 | (ri & ConstU32(0xFFFF0000)) >> 16;
    ri = cast<int32_t>(ri >> (32 - log2(n)));
    return ri;
}

Func fft(Func in, const int32_t n, const int32_t batch_size)
{
    Var c{"c"}, i{"i"}, k{"k"};

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
    Expr ri = bit_reverse(i, n);
    stage = BoundaryConditions::repeat_edge(stage, {{0, 2}, {0, n}, {0, batch_size}});

    Func out("out");
    out(c, i, k) = stage(c, ri, k);

    schedule(weight, {2, n/2});

    return out;
}

Func copy(Func src)
{
    Var x, y;

    Func dst{"dst"};
    dst(x, y) = src(x, y);

    return dst;
}

} // anonymous
} // Element
} // Halide

#endif
