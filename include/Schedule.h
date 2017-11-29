#pragma once

#include <cassert>
#include <vector>

#include <Halide.h>

namespace Halide {
namespace Element {

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

    auto bounds = f.function().schedule().bounds();
    for (size_t i=0; i<mins.size(); ++i) {
        Var var = f.args()[i];
        if (std::find_if(bounds.begin(), bounds.end(), [&](const Internal::Bound& b) { return b.var == var.name(); }) == bounds.end()) {
            f.bound(var, mins[i], extents[i]);
        }
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

} // Element
} // Halide
