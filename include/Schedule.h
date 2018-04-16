#pragma once

#include <cassert>
#include <vector>

#include <Halide.h>

namespace Halide {
namespace Element {

namespace {

//
// Scheduling
//
ImageParam& schedule(ImageParam& ip, const std::vector<int32_t>& mins, const std::vector<int32_t>& extents)
{
    assert(mins.size() == extents.size());
    for (size_t i=0; i<mins.size(); ++i) {
        ip.dim(i).set_bounds(mins[i], extents[i]);
    }
    return ip;
}

Func& schedule(Func& f, const std::vector<int32_t>& mins, const std::vector<int32_t>& extents)
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

ImageParam& schedule(ImageParam& ip, const std::vector<int32_t>& shape)
{
    const std::vector<int32_t> mins(shape.size(), 0);
    return schedule(ip, mins, shape);
}

Func& schedule(Func& f, const std::vector<int32_t>& shape)
{
    const std::vector<int32_t> mins(shape.size(), 0);
    return schedule(f, mins, shape);
}

Func& schedule_burst(Func& f, const std::vector<int32_t>& shape, int32_t burst_num)
{
    schedule(f, shape);
#if defined(HALIDE_FOR_FPGA)
    f.hls_burst(burst_num);
#endif
    return f;
}

ImageParam& schedule_burst(ImageParam& ip, int32_t burst_num)
{
#if defined(HALIDE_FOR_FPGA)
    ip.hls_burst(burst_num);
#endif
    return ip;
}

Buffer<>& schedule_burst(Buffer<>& b, int32_t burst_num)
{
#if defined(HALIDE_FOR_FPGA)
    b.hls_burst(burst_num);
#endif
    return b;
}

Func& schedule_memory(Func& f, const std::vector<int32_t>& shape)
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

}
} // Element
} // Halide
