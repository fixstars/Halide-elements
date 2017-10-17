#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "gaussian_u8.h"
#include "gaussian_u16.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, int32_t _width, int32_t _height, int32_t _window_width, int32_t _window_height, double _sigma, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const int window_width = 3;
        const int window_height = 3;
        const double sigma = 1.0;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, width, height, window_width, window_height, sigma, output);

        double kernel_sum = 0;
        for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
            for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                kernel_sum += exp(-(i * i + j * j) / (2 * sigma * sigma));
            }
        }

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                double expect_f = 0.0f;
                for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                    int yy = std::min(std::max(0, y + j), height - 1);
                    for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
                        int xx = std::min(std::max(0, x + i), width - 1);
                        expect_f += exp(-(i * i + j * j) / (2 * sigma * sigma)) * input(xx, yy);
                    }
                }
                expect_f /= kernel_sum;
                T expect = round_to_nearest_even<T>(expect_f);
                T actual = output(x, y);
                if (expect != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d, expect_f = %f", x, y, expect, x, y, actual, expect_f).c_str());
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
    test<uint8_t>(gaussian_u8);
    test<uint16_t>(gaussian_u16);
}
