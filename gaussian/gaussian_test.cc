#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "gaussian.h"

#include "testcommon.h"

using std::string;
using Halide::Runtime::Buffer;

int main()
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
        const float sigma = 1.0;
        Buffer<uint8_t> input(width, height);
        Buffer<uint8_t> output(width, height);
        std::srand(time(NULL));
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                input(x, y) = static_cast<uint8_t>(std::rand());
                output(x, y) = 0;
            }
        }

        gaussian(input, window_width, window_height, sigma, output);

        float kernel_sum = 0;
        for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
            for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                kernel_sum += exp(-(i * i + j * j) / (2 * sigma * sigma));
            }
        }

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                float expect_f = 0.0f;
                for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                    int yy = y + j >= 0 ? y + j: 0;
                    yy = yy < height ? yy : height - 1;
                    for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
                        int xx = x + i >= 0 ? x + i: 0;
                        xx = xx < width ? xx : width - 1;
                        expect_f += exp(-(i * i + j * j) / (2 * sigma * sigma)) * input(xx, yy);
                    }
                }
                expect_f /= kernel_sum;
                uint8_t expect = round_to_nearest_even<uint8_t>(expect_f);
                uint8_t actual = output(x, y);
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
