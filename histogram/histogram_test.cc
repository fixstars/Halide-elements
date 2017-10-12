#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "histogram.h"

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
        const int hist_width = std::numeric_limits<uint8_t>::max() + 1;
        Buffer<uint8_t> input(width, height);
        Buffer<uint32_t> output(hist_width);
        uint32_t expect[hist_width];
        uint32_t hist_size = std::numeric_limits<uint8_t>::max() + 1;
        uint32_t hist[hist_size];
        int bin_size = (hist_size + hist_width - 1) / hist_width;

        std::srand(time(NULL));
        memset(hist, 0, sizeof(hist));
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                input(x, y) = static_cast<uint8_t>(std::rand());
                hist[input(x, y)]++;
            }
        }

        int idx = 0;
        for (int i = 0; i < hist_width; i++) {
            uint32_t sum = 0;
            for (int k = 0; k < bin_size && idx < hist_size; ++k) {
                sum += hist[idx++];
            }
            expect[i] = sum;
        }

        for (int x=0; x<hist_width; ++x) {
            output(x) = 0;
        }

        histogram(input, output);

        for (int x=0; x<hist_width; ++x) {
            uint32_t actual = output(x);
            if (expect[x] != actual) {
                throw std::runtime_error(format("Error: expect(%d) = %d, actual(%d) = %d", x, expect[x], x, actual).c_str());
            }
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}
