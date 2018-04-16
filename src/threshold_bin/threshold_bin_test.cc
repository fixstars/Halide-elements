#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "threshold_bin_u8.h"
#include "threshold_bin_u16.h"

#include "test_common.h"

template<typename T>
Halide::Runtime::Buffer<T>& bin_ref(Halide::Runtime::Buffer<T>& dst,
                                const Halide::Runtime::Buffer<T>& src,
                                const int32_t width, const int32_t height,
                                const T threshold, const T value)
{
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            dst(x, y) = src(x, y) > threshold ? value : 0;
        }
    }
    return dst;
}

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                     T _threshold, T _value,
                     struct halide_buffer_t *_dst_buffer))
{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};

        const T threshold = mk_rand_scalar<T>();
        const T value = mk_rand_scalar<T>();
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, threshold, value, output);
        auto expect = mk_rand_buffer<T>(extents);
        expect = bin_ref(expect, input, width, height, threshold, value);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T actual = output(x, y);
                if (expect(x, y) != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                 x, y, expect(x, y), x, y, actual).c_str());
                 }

            }
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    printf("Success!\n");
    return 0;
}

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(threshold_bin_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(threshold_bin_u16);
#endif
}
