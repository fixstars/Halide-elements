#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "histogram2d.h"

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
        const int hist_width = std::numeric_limits<uint8_t>::max() + 1;
        const std::vector<int32_t> extents{width, height}, extents_hist{hist_width, hist_width};
        auto input0 = mk_rand_buffer<uint8_t>(extents);
        auto input1 = mk_rand_buffer<uint8_t>(extents);
        auto output = mk_null_buffer<uint32_t>(extents_hist);
        uint32_t expect[hist_width][hist_width];

        memset(expect, 0, sizeof(expect));
        double step =
            hist_width / (static_cast<double>((std::numeric_limits<uint8_t>::max)()) + 1.0);
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                int i_idx =
                    static_cast<int>(std::floor((static_cast<double>(input0(x, y))) * step));
                int j_idx =
                    static_cast<int>(std::floor((static_cast<double>(input1(x, y))) * step));
                if (i_idx < hist_width && j_idx < hist_width) {
                    expect[i_idx][j_idx]++;
                }
            }
        }

        histogram2d(input0, input1, output);

        for (int y=0; y<hist_width; ++y) {
            for (int x=0; x<hist_width; ++x) {
                uint32_t actual = output(x, y);
                if (expect[x][y] != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect[x][y], x, y, actual).c_str());
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
