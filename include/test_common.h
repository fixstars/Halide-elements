#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <limits>
#include <random>
#include <string>
#include <vector>

#include "HalideBuffer.h"
#include "HalideRuntime.h"

namespace {

template<typename... Rest>
std::string format(const char *fmt, const Rest&... rest)
{
    int length = snprintf(NULL, 0, fmt, rest...) + 1; // Explicit place for null termination
    std::vector<char> buf(length, 0);
    snprintf(&buf[0], length, fmt, rest...);
    std::string s(buf.begin(), std::find(buf.begin(), buf.end(), '\0'));
    return s;
}


template<typename T>
Halide::Runtime::Buffer<T> mk_rand_int_buffer(const std::vector<int32_t>& extents)
{
    Halide::Runtime::Buffer<T> buf(extents);
    const int32_t size = std::accumulate(extents.begin(), extents.end(), 1, std::multiplies<int32_t>());

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

    for (int32_t i=0; i<size; ++i) {
        *(reinterpret_cast<T*>(buf.data())+i) = dist(mt);
    }

    return buf;
}

template<typename T>
Halide::Runtime::Buffer<T> mk_rand_real_buffer(const std::vector<int32_t>& extents, 
                                               T min_value = std::numeric_limits<T>::min(), 
                                               T max_value = std::numeric_limits<T>::max())
{
    Halide::Runtime::Buffer<T> buf(extents);
    const int32_t size = std::accumulate(extents.begin(), extents.end(), 1, std::multiplies<int32_t>());

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<T> dist(min_value, max_value);

    for (int32_t i=0; i<size; ++i) {
        *(reinterpret_cast<T*>(buf.data())+i) = dist(mt);
    }

    return buf;
}

template<typename T>
Halide::Runtime::Buffer<T> mk_rand_buffer(const std::vector<int32_t>& extents)
{
    return mk_rand_int_buffer<T>(extents);
}

template<>
Halide::Runtime::Buffer<float> mk_rand_buffer<float>(const std::vector<int32_t>& extents)
{
    return mk_rand_real_buffer<float>(extents);
}

template<>
Halide::Runtime::Buffer<double> mk_rand_buffer<double>(const std::vector<int32_t>& extents)
{
    return mk_rand_real_buffer<double>(extents);
}

template<typename T>
Halide::Runtime::Buffer<T> mk_null_buffer(const std::vector<int32_t>& extents)
{
    return Halide::Runtime::Buffer<T>(extents);
}

template<typename T>
Halide::Runtime::Buffer<T> mk_const_buffer(const std::vector<int32_t>& extents, const T val)
{
    Halide::Runtime::Buffer<T> buf(extents);
    const int32_t size = std::accumulate(extents.begin(), extents.end(), 1, std::multiplies<int32_t>());

    for (int32_t i=0; i<size; ++i) {
        *(reinterpret_cast<T*>(buf.data())+i) = val;
    }

    return buf;
}

template<typename T>
T round_to_nearest_even(float v)
{
    float i;
    float f = modff(v, &i);
    if (0.5 == f || -0.5 == f) {
        if (0 == static_cast<int64_t>(i) % 2) {
            return static_cast<T>(i);
        } else {
            if (v < 0) {
                return static_cast<T>(i - 1);
            } else {
                return static_cast<T>(i + 1);
            }
        }
    } else {
        return static_cast<T>(round(v));
    }
}

}

#endif /* TEST_COMMON_H */
