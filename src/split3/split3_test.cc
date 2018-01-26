#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "split3_u8.h"
#include "split3_u16.h"
#include "split3_i8.h"
#include "split3_i16.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                     struct halide_buffer_t *_dst_buffer0,
                     struct halide_buffer_t *_dst_buffer1,
                     struct halide_buffer_t *_dst_buffer2))
{
    try {
        constexpr unsigned int N = 3;
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> in_extents{N, width, height};
        const std::vector<int32_t> out_extents{width, height};
        auto input = mk_null_buffer<T>(in_extents);
        Halide::Runtime::Buffer<T> output[N];
        for (int i = 0; i < N; ++i) {
            output[i] = mk_rand_buffer<T>(out_extents);
        }
        T expect[N][height * width]; //expected output: 3 h*w images

        //ref
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                for (int c = 0; c < N; c++) {
                    expect[c][y * width+ x] = input(c, x, y);
                }
            }
        }
        func(input, output[0], output[1], output[2]);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                for (int c = 0; c < N; c++) {
                    T actual = output[c](x, y);
                    if (expect[c][y * width + x] != actual) {
                        throw std::runtime_error(format("Error: expect(%d, %d, %d) = %u, actual(%d, %d, %d) = %u",
                                                        x, y, c, expect[c][y * width + x], x, y, c, actual).c_str());
                     }
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
    test<uint8_t>(split3_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(split3_u16);
#endif
#ifdef TYPE_i8
    test<int8_t>(split3_i8);
#endif
#ifdef TYPE_i16
    test<int16_t>(split3_i16);
#endif
}
