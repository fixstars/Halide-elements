#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "median_u8.h"
#include "median_u16.h"

#include "test_common.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_dst_buffer))
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
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, output);
        T expect[height][width];

        
        const int offset_x = window_width / 2;
        const int offset_y = window_height / 2;
        const int window_area = window_width * window_height;
        size_t table_size = static_cast<size_t>(window_area);
        T table[table_size];
        
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                for (int k = -offset_y; k <= offset_y; k++) {
                    for (int l = -offset_x; l <= offset_x; l++) {
                        table[(k + offset_y) * window_width + (l + offset_x)] =
                            input(BORDER_INTERPOLATE(j + l, width), BORDER_INTERPOLATE(i + k, height));
                    }
                }
                std::sort(table, table + table_size);
                expect[i][j] = table[table_size / 2];
            }
        }
        func(input, output);
        
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T expect_ = expect[y][x];
                T actual = output(x, y);
                if (expect_ != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect_, x, y, actual).c_str());
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
    test<uint8_t>(median_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(median_u16);
#endif
}
