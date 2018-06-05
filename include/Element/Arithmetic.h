#ifndef HALIDE_ELEMENT_ARITHMETIC_H
#define HALIDE_ELEMENT_ARITHMETIC_H

#include "Halide.h"
#include "Util.h"
#include<cstdio>

namespace Halide {
namespace Element {

namespace {

template<typename T, typename D>
Func sq_sum(ImageParam src, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};

    Func dst("sq_sum");

    RDom r(0, width, 0, height);

    dst(x, y) = cast<D>(sum(cast<double>(src(r.x, r.y)) * cast<double>(src(r.x, r.y))));

    return dst;
}

template<typename T, typename D>
Func sum(ImageParam src, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};

    Func dst("sum");

    RDom r(0, width, 0, height);

    dst(x, y) = cast<D>(sum(cast<typename SumType<T>::type>(src(r.x, r.y))));

    return dst;
}

template<typename T>
Func add(Func src0, Func src1)
{
    using upper_t = typename Upper<T>::type;

    Var x{"x"}, y{"y"};

    Expr srcval0 = cast<upper_t>(src0(x, y)), srcval1 = cast<upper_t>(src1(x, y));
    Expr dstval = min(srcval0 + srcval1, cast<upper_t>(type_of<T>().max()));

    Func dst;
    dst(x, y) = cast<T>(dstval);

    return dst;
}

template<typename T>
Func add_scalar(Func src, Expr val)
{
    Var x{"x"}, y{"y"};

    Expr dstval = clamp(round(cast<double>(src(x, y)) + val), 0, cast<double>(type_of<T>().max()));

    Func dst;
    dst(x, y) = cast<T>(dstval);

    return dst;
}

Func calc_and(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = src0(x, y) & src1(x, y);

    return dst;
}

Func and_scalar(Func src0, Expr val)
{
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = src0(x, y) & val;

    return dst;
}

template<typename T>
Func average(ImageParam src, int32_t window_width, int32_t window_height)
{
    using upper_t = typename Upper<T>::type;

    Var x{"x"}, y{"y"};

    Func clamped = BoundaryConditions::repeat_edge(src);
    Expr w_half = div_round_to_zero(window_width, 2);
    Expr h_half = div_round_to_zero(window_height, 2);

    RDom r(-w_half, window_width, -h_half, window_height);

    Func dst;
    dst(x, y) = cast<T>(round(cast<float>(sum(cast<upper_t>(clamped(x + r.x, y + r.y)))) / cast<float>(window_width * window_height)) + 0.5f);

    return dst;
}

Func multiply(Func src1, Func src2)
{
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = src1(x, y) * src2(x, y);

    return dst;
}

template<typename T>
Func mul_scalar(Func src0, Expr val)
{
    Var x{"x"}, y{"y"};

    Expr dstval = min(src0(x, y) * val, cast<float>(type_of<T>().max()));
    dstval = max(dstval, 0);

    Func dst;
    dst(x, y) = cast<T>(round(dstval));

    return dst;
}

template<typename T>
Func div_scalar(Func src, Expr val)
{
    Var x{"x"}, y{"y"};

    Expr srcval = src(x, y);
    Expr dstval = max(min(srcval / val, cast<double>(type_of<T>().max())), 0);

    Func dst;
    dst(x, y) = cast<T>(round(dstval));

    return dst;
}

template<typename T>
Func nand(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = ~(src0(x, y) & src1(x, y));

    return dst;
}

template<typename T>
Func nor(Func src0, Func src1) {
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = ~(src0(x, y) | src1(x, y));

    return dst;
}

Func min(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = min(src0(x, y), src1(x, y));

    return dst;
}

Func min_pos(Func src, int32_t width, int32_t height)
{
    Var x{"x"};
    RDom r{0, width, 0, height, "r"};

    Func res{"res"};
    res(x) = argmin(r, src(r.x, r.y));
    schedule(res, {1});

    Var d{"d"};
    Func dst{"dst"};
    dst(d) = cast<uint32_t>(0);
    dst(0) = cast<uint32_t>(res(0)[0]);
    dst(1) = cast<uint32_t>(res(0)[1]);

    return dst;
}

template<typename T>
Func min_value(Func src, Func roi, int32_t width, int32_t height)
{
    Var x{"x"};
    Func count{"count"}, dst;
    RDom r{0, width, 0, height, "r"};
    r.where(roi(r.x, r.y) != 0);

    count(x) = sum(select(roi(r.x, r.y) == 0, 0, 1));
    schedule(count, {1});

    dst(x) = cast<T>(select(count(x) == 0, 0, minimum(src(r.x, r.y))));

    return dst;
}

Func max(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst;
    dst(x, y) = max(src0(x, y), src1(x, y));

    return dst;
}

Func max_pos(Func src, int32_t width, int32_t height)
{
    Var x{"x"};
    RDom r{0, width, 0, height, "r"};

    Func res{"res"};
    res(x) = argmax(r, src(r.x, r.y));
    schedule(res, {1});

    Var d{"d"};
    Func dst;
    dst(d) = cast<uint32_t>(0);
    dst(0) = cast<uint32_t>(res(0)[0]);
    dst(1) = cast<uint32_t>(res(0)[1]);

    return dst;
}

template<typename T>
Func max_value(Func src, Func roi, int32_t width, int32_t height)
{
    Var x{"x"};
    Func count{"count"}, dst;
    RDom r{0, width, 0, height, "r"};
    r.where(roi(r.x, r.y) != 0);

    count(x) = sum(select(roi(r.x, r.y) == 0, 0, 1));
    schedule(count, {1});

    dst(x) = cast<T>(select(count(x) == 0, 0, maximum(src(r.x, r.y))));

    return dst;
}

template<typename T>
Func equal(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst{"dst"};
    dst(x, y) = cast<T>(select(src0(x, y) == src1(x, y), type_of<T>().max(), 0));

    return dst;
}

template<typename T>
Func cmpgt(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst{"dst"};
    dst(x, y) = cast<T>(select(src0(x, y) > src1(x, y), type_of<T>().max(), 0));

    return dst;
}

template<typename T>
Func cmpge(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};

    Func dst{"dst"};
    dst(x, y) = cast<T>(select(src0(x, y) >= src1(x, y), type_of<T>().max(), 0));;

    return dst;
}

template<typename T>
Func integral(Func in, int32_t width, int32_t height)
{
    Var x{"x"}, y{"y"};
    Func dst{"dst"}, integral{"integral"};
    integral(x, y) = cast<uint64_t>(in(x, y));

    RDom r1{1, width - 1, 0, height, "r1"};
    integral(r1.x, r1.y) += integral(r1.x - 1, r1.y);

    RDom r2{0, width, 1, height - 1, "r2"};
    integral(r2.x, r2.y) += integral(r2.x, r2.y - 1);
    schedule(integral, {width, height});

    dst(x, y) = cast<T>(integral(x, y));

    return dst;
}


template<typename T>
Func histogram(Func src, int32_t width, int32_t height, int32_t hist_width)
{
    uint32_t hist_size = static_cast<uint32_t>(std::numeric_limits<T>::max()) + 1;
    int32_t bin_size = (hist_size + hist_width - 1) / hist_width;

    Var x{"x"};
    RDom r{0, width, 0, height};

    Func dst;
    dst(x) = cast<uint32_t>(0);

    Expr idx = cast<int32_t>(src(r.x, r.y) / bin_size);
    dst(idx) += cast<uint32_t>(1);

    return dst;
}

template<typename T>
Func histogram2d(Func src0, Func src1, int32_t width, int32_t height, int32_t hist_width)
{
    Var x{"x"}, y{"y"};
    RDom r{0, width, 0, height};

    Func dst;
    dst(x, y) = cast<uint32_t>(0);
    Expr idx0 = cast<int32_t>(src0(r.x, r.y) * cast<uint64_t>(hist_width) / (cast<uint64_t>(type_of<T>().max()) + 1));
    Expr idx1 = cast<int32_t>(src1(r.x, r.y) * cast<uint64_t>(hist_width) / (cast<uint64_t>(type_of<T>().max()) + 1));
    dst(idx0, idx1) += cast<uint32_t>(1);

    return dst;
}

template<typename T>
Func sub(Func src0, Func src1)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = cast<T>(select(src0(x, y) > src1(x,y), src0(x, y)-src1(x,y), 0));
    return dst;
}

Func filter_xor(Func src0, Func src1) {
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = src0(x, y) ^ src1(x, y);
    return dst;
}

template<typename T>
Func sq_integral(Func src, int32_t width, int32_t height){
    Var x{"x"}, y{"y"};
    Func dst{"dst"};
    dst(x, y) = cast<T>(src(x, y)) * cast<T>(src(x, y));

    RDom h{1, width-1, 0, height, "h"};
    dst(h.x, h.y) += dst(h.x-1, h.y);

    RDom v{0, width, 1, height-1, "v"};
    dst(v.x, v.y) += dst(v.x, v.y-1);
    return dst;
}


template<typename T>
Func sub_scalar(Func src, Expr val)
{
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = cast<T>(clamp(round(cast<double>(src(x, y)) - val), 0, cast<double>(type_of<T>().max())));

    return dst;
}

template<typename T>
Func average_value(Func src, Func roi, int32_t width, int32_t height)
{
    Var x{"x"};
    Func count{"count"}, dst{"dst"};
    RDom r{0, width, 0, height, "r"};
    r.where(roi(r.x, r.y) != 0);

    count(x) = sum(select(roi(r.x, r.y) == 0, 0, 1));
    schedule(count, {1});
    dst(x) = cast<T>(select(count(x)==0, 0, sum(cast<double>(src(r.x, r.y)))/count(x)));
        return dst;
}

Func filter_or(Func src0, Func src1) {
    Var x{"x"}, y{"y"};
    Func dst;
    dst(x, y) = src0(x, y) | src1(x, y);

    return dst;
}



} // anonymous

} // element
} // Halide

#endif
