#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "div_scalar.h"

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
        const float value = 2.0;
        Buffer<uint8_t> input(width, height);
        Buffer<uint8_t> output(width, height);
        std::srand(time(NULL));
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                input(x, y) = static_cast<uint8_t>(std::rand());
                output(x, y) = 0;
            }
        }

        div_scalar(input, value, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t expect = input(x, y);
                uint8_t actual = output(x, y);
                float f = expect / value;
                f = std::min(static_cast<float>(std::numeric_limits<uint8_t>::max()), f);
                f = std::max(0.0f, f);
                expect = round_to_nearest_even<uint8_t>(f);
                if (expect != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect, x, y, actual).c_str());
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
