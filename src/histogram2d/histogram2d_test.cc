#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "histogram2d_u8.h"
#include "histogram2d_u16.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src0_buffer, struct halide_buffer_t *_src1_buffer, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const int hist_width = 256;
        const std::vector<int32_t> extents{width, height}, extents_hist{hist_width, hist_width};
        auto input0 = mk_rand_buffer<T>(extents);
        auto input1 = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<uint32_t>(extents_hist);
        std::vector<uint32_t> expect(hist_width * hist_width);

        double step =
            hist_width / (static_cast<double>((std::numeric_limits<T>::max)()) + 1.0);
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                int i_idx =
                    static_cast<int>(std::floor((static_cast<double>(input0(x, y))) * step));
                int j_idx =
                    static_cast<int>(std::floor((static_cast<double>(input1(x, y))) * step));
                if (i_idx < hist_width && j_idx < hist_width) {
                    expect[i_idx + j_idx * hist_width]++;
                }
            }
        }

        func(input0, input1, output);

        for (int y=0; y<hist_width; ++y) {
            for (int x=0; x<hist_width; ++x) {
                uint32_t actual = output(x, y);
                if (expect[x + y * hist_width] != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect[x + y * hist_width], x, y, actual).c_str());
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
    test<uint8_t>(histogram2d_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(histogram2d_u16);
#endif
}
