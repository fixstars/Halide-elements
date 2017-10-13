#include <cstdlib>
#include <fstream>
#include <iostream>

#include <unistd.h>
#include <dlfcn.h>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "simple_isp.h"

#include "test_common.h"

using namespace Halide::Runtime;

int main(int argc, char **argv) {
    try {
        const int width = 3280;
        const int height = 2486;

        Buffer<uint16_t> input = mk_const_buffer<uint16_t>({3280, 2486}, 64);
        Buffer<uint8_t> output(4, width, height);

        const uint16_t optical_black_clamp_value = 16;
        const float gamma_value = 1.0f/1.8f;
        const float saturation_value = 0.6f;
        
        simple_isp(input, optical_black_clamp_value, gamma_value, saturation_value, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                for (int c=0; c<4; ++c) {
                    uint8_t ev = c < 3 ? 48 : 0;
                    uint8_t av = output(c, x, y);
                    if (ev != av) {
                        throw std::runtime_error(format("Error: expect(%d, %d, %d) = %d, actual(%d, %d, %d) = %d", c, x, y, ev, c, x, y, av).c_str());
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
