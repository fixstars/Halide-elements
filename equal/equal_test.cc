#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "equal.h"

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
        Buffer<uint8_t> input0(width, height), input1(width, height);
        Buffer<uint8_t> output(width, height);
        std::srand(time(NULL));
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                input0(x, y) = static_cast<uint8_t>(std::rand());
                input1(x, y) = static_cast<uint8_t>(std::rand());
                output(x, y) = 0;
            }
        }

        equal(input0, input1, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t expect = input0(x, y) == input1(x, y) ? std::numeric_limits<uint8_t>::max() : 0;
                uint8_t actual = output(x, y);
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
