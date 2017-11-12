#ifndef HALIDE_ELEMENT_SCHEDULE_H
#define HALIDE_ELEMENT_SCHEDULE_H

#include <Halide.h>

namespace Halide {
namespace Element {

// 
// Sdheculing
//
ImageParam& schedule(ImageParam& ip, const std::vector<int32_t>& shape)
{
    for (size_t i=0; i<shape.size(); ++i) {
        ip.dim(i).set_bounds(0, shape[i]);
    }
    return ip;
}

Func& schedule(Func& f, const std::vector<int32_t>& shape)
{
    f.compute_root();
    for (size_t i=0; i<shape.size(); ++i) {
        f.bound(f.args()[i], 0, shape[i]);
    }
    return f;
}

} // Element
} // Halide

#endif