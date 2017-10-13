#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "div_scalar.h"

#include "test_common.h"

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
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<uint8_t>(extents);
        auto output = mk_null_buffer<uint8_t>(extents);

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
