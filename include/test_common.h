#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <cstring>
#include <exception>
#include <fstream>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "HalideBuffer.h"
#include "HalideRuntime.h"

#include "Element/Util.h"

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

static std::random_device rd;
static std::mt19937 mt(rd());

template<typename T>
T mk_rand_int_scalar()
{
    std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

    return dist(mt);
}

template<typename T>
T mk_rand_real_scalar()
{
    std::uniform_real_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

    return dist(mt);
}

template<typename T>
T mk_rand_scalar()
{
    return mk_rand_int_scalar<T>();
}

template<>
float mk_rand_scalar<float>()
{
    return mk_rand_real_scalar<float>();
}

template<>
double mk_rand_scalar<double>()
{
    return mk_rand_real_scalar<double>();
}

template<typename T>
Halide::Runtime::Buffer<T> mk_rand_int_buffer(const std::vector<int32_t>& extents)
{
    Halide::Runtime::Buffer<T> buf(extents);
    const int32_t size = std::accumulate(extents.begin(), extents.end(), 1, std::multiplies<int32_t>());

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

Halide::Runtime::Buffer<uint8_t> load_pgm(const std::string& fname)
{
    FILE *fd = fopen(fname.c_str(), "rb");
    if (fd == NULL) {
        throw std::runtime_error("File not found");
    }

    char header[256];
    fscanf(fd, "%s\n", header);
    int32_t width=0, height=0;
    fscanf(fd, "%d %d\n", &width, &height);
    int32_t max_depth;
    fscanf(fd, "%d\n", &max_depth);
    if (max_depth != 255) {
        fclose(fd);
        throw std::runtime_error("Invalid file");
    }
    Halide::Runtime::Buffer<uint8_t> im(width, height);
    fread(reinterpret_cast<char *>(im.data()), sizeof(uint8_t), width*height, fd);
    fclose(fd);
    return im;
}

void save_ppm(const std::string& fname, Halide::Runtime::Buffer<uint8_t>& buffer)
{
    std::ofstream ofs(fname.c_str());
    if (!ofs.is_open()) {
        throw std::runtime_error("File not found");
    }
    if (buffer.dimensions() != 3 || (buffer.extent(0) != 3 && buffer.extent(0) != 4)) {
        throw std::runtime_error("Invalid buffer");
    }

    uint32_t width = buffer.extent(1);
    uint32_t height = buffer.extent(2);
    std::vector<uint8_t> buf(3*width*height);
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            for (int c=0; c<3; ++c) {
                buf[y*3*width+x*3+c] = buffer(c, x, y);
            }
        }
    }

    ofs << "P6" << std::endl;
    ofs << width << " " << height << std::endl;
    ofs << "255" << std::endl;

    ofs.write(reinterpret_cast<char *>(buf.data()), 3*width*height*sizeof(uint8_t));
}


template<typename T>
T round_to_nearest_even(double v)
{
    double i;
    double f = modf(v, &i);
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

} //anonymous namespace

#endif /* TEST_COMMON_H */
