#include <cstdlib>
#include <iostream>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "convolution.h"

#include "test_common.h"

using namespace Halide::Runtime;

int main(int argc, char **argv) {
    try {

        const int width = 512;
        const int height = 512;
        Buffer<uint8_t> input = mk_const_buffer<uint8_t>({width, height}, 1);
                
        using fixed16_t = int16_t;
        constexpr uint32_t frac_bits = 10;
        const fixed16_t kv = static_cast<fixed16_t>(round(1.0f * (1 << frac_bits)));
        fixed16_t kernel_data[5][5] = {
            { kv, kv, kv, 0, 0},
            { kv, kv, kv, 0, 0},
            { kv, kv, kv, 0, 0},
            { 0,  0,  0,  0, 0},
            { 0,  0,  0,  0, 0}
        };

        Buffer<fixed16_t> kernel(reinterpret_cast<fixed16_t*>(kernel_data), 5, 5);

        Buffer<uint8_t> output(width, height);

        convolution(input, kernel, 3, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t ev = input(x, y) * 9;
                uint8_t av = output(x, y);
                if (ev != av) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, ev, x, y, av).c_str());
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
