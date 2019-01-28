#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <exception>

namespace Halide {
namespace Element {

namespace {

void throw_error(const char *msg)
{
    throw std::runtime_error(msg);
}

void throw_assert(bool condition, const char *msg)
{
    if (!condition) {
        throw_error(msg);
    }
}

template<typename T>
struct Upper;

template<>
struct Upper<uint8_t> {
    using type = uint16_t;
};

template<>
struct Upper<uint16_t> {
    using type = uint32_t;
};

template<>
struct Upper<uint32_t> {
    using type = uint64_t;
};

template<>
struct Upper<int16_t> {
    using type = int32_t;
};

template<>
struct Upper<int32_t> {
    using type = int64_t;
};

template<>
struct Upper<float> {
    using type = float;
};

template<>
struct Upper<double> {
    using type = double;
};

template <typename T>
struct SumType {
    using type = uint64_t;
};

template <>
struct SumType<float> {
    using type = double;
};

template <>
struct SumType<double> {
    using type = double;
};

} // anonymous
} // Element
} // Halide
