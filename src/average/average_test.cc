#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "average_u8.h"
#include "average_u16.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_dst_buffer))
{

    try {
        using upper_t = typename Halide::Element::Upper<T>::type;

        int ret = 0;

        //
        // Run
        //
        const int32_t width = 1024;
        const int32_t height = 768;
        const int32_t window_width = 3;
        const int32_t window_height = 3;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, output);

        const int32_t wx_lower = -window_width / 2;
        const int32_t wx_upper = wx_lower + window_width;
        const int32_t wy_lower = -window_height / 2;
        const int32_t wy_upper = wy_lower + window_height;
        const int32_t window_area = window_width * window_height;

        for (int32_t y = 0; y < height; ++y) {
            for (int32_t x = 0; x < width; ++x) {
                int32_t ax, ay;
                upper_t f = 0;

                for (int32_t wy = wy_lower; wy < wy_upper; wy++) {
                    for (int32_t wx = wx_lower; wx < wx_upper; wx++) {
                        ax = x + wx;
                        ay = y + wy;

                        if (ax < 0) ax = 0;
                        if (ay < 0) ay = 0;
                        if (width <= ax) ax = width - 1;
                        if (height <= ay) ay = height - 1;

                        f += input(ax, ay);

                    }
                }
                T expect = static_cast<T>(roundf(static_cast<float>(f) / static_cast<float>(window_area)));

                T actual = output(x, y);

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

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(average_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(average_u16);
#endif
}
