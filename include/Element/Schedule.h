#pragma once

#include <cassert>
#include <vector>

#include <Halide.h>
#include "Util.h"

namespace Halide {
namespace Element {
namespace {

//
// Scheduling
//
ImageParam& schedule(ImageParam& ip, const std::vector<Expr>& mins, const std::vector<Expr>& extents)
{
    assert(mins.size() == extents.size());
    for (size_t i=0; i<mins.size(); ++i) {
        ip.dim(i).set_bounds(mins[i], extents[i])
          .dim(i).set_stride(i == 0 ? 1 : ip.dim(i - 1).stride() * extents[i - 1]);
    }
    return ip;
}

Func& schedule(Func& f, const std::vector<Expr>& mins, const std::vector<Expr>& extents)
{
    assert(mins.size() == extents.size());

    f.compute_root();

    // auto bounds = f.function().schedule().bounds();
    for (size_t i=0; i<mins.size(); ++i) {
        Var var = f.args()[i];
        // NOTE: Internal API should not be used. It is responsible to user to scheduled just once.
        // if (std::find_if(bounds.begin(), bounds.end(), [&](const Internal::Bound& b) { return b.var == var.name(); }) == bounds.end()) {
        //     f.bound(var, mins[i], extents[i]);
        // }
        f.bound(var, mins[i], extents[i]);
    }
    return f;
}

ImageParam& schedule(ImageParam& ip, const std::vector<Expr>& shape)
{
    const std::vector<Expr> mins(shape.size(), 0);
    return schedule(ip, mins, shape);
}

Func& schedule(Func& f, const std::vector<Expr>& shape)
{
    const std::vector<Expr> mins(shape.size(), 0);
    return schedule(f, mins, shape);
}

Func& schedule_burst(Func& f, const std::vector<Expr>& shape, Expr burst_num)
{
    schedule(f, shape);
#if defined(HALIDE_FOR_FPGA)
    Expr bn = simplify(burst_num);
    throw_assert(is_const(bn), "burst_num should be constant value.");
    f.hls_burst(static_cast<int32_t>(*as_const_int(bn)));
#endif
    return f;
}

ImageParam& schedule_burst(ImageParam& ip, Expr burst_num)
{
#if defined(HALIDE_FOR_FPGA)
    Expr bn = simplify(burst_num);
    throw_assert(is_const(bn), "burst_num should be constant value.");
    ip.hls_burst(static_cast<int32_t>(*as_const_int(bn)));
#endif
    return ip;
}

Buffer<>& schedule_burst(Buffer<>& b, Expr burst_num)
{
#if defined(HALIDE_FOR_FPGA)
    Expr bn = simplify(burst_num);
    throw_assert(is_const(bn), "burst_num should be constant value.");
    b.hls_burst(static_cast<int32_t>(*as_const_int(bn)));
#endif
    return b;
}

Func& schedule_memory(Func& f, const std::vector<Expr>& shape)
{
    schedule(f, shape);
#if defined(HALIDE_FOR_FPGA)
    f.hls_interface(Interface::Type::Memory);
#endif
    return f;
}

ImageParam& schedule_memory(ImageParam& ip)
{
#if defined(HALIDE_FOR_FPGA)
    ip.hls_interface(Interface::Type::Memory);
#endif
    return ip;
}

ImageParam& set_extent_interval(ImageParam& ip, const std::vector<std::pair<int32_t, int32_t>>& intervals)
{
    assert(static_cast<size_t>(ip.dimensions()) >= intervals.size());
    for (size_t i=0; i<intervals.size(); ++i) {
        ip.dim(i).set_min(0);
#if defined(HALIDE_FOR_FPGA)
        ip.dim(i).set_extent_interval(Halide::Internal::Interval(intervals[i].first, intervals[i].second));
#endif
    }
    return ip;
}

Func& set_extent_interval(Func& f, const std::vector<std::pair<int32_t, int32_t>>& intervals)
{
    assert(static_cast<size_t>(f.dimensions()) >= intervals.size() && f.output_buffer().defined());
    for (size_t i=0; i<intervals.size(); ++i) {
        f.output_buffer().dim(i).set_min(0);
#if defined(HALIDE_FOR_FPGA)
        f.output_buffer().dim(i).set_extent_interval(Halide::Internal::Interval(intervals[i].first, intervals[i].second));
#endif
    }
    return f;
}

} // anonymous
} // Element
} // Halide
