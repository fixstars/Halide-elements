#include <iostream>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "sobel_u8.h"
#include "sobel_u16.h"

#include "test_common.h"
#include "cstdlib"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_input_buffer, struct halide_buffer_t *_output_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);
        T expect[width][height];

        for (int y=0; y<height; ++y) {
            int yy0 = std::max(0, y - 1); //protect out of bounds
            int yy1 = y;
            int yy2 = std::min(y + 1, height - 1);
            for (int x=0; x<width; ++x) {
                int xx0 = std::max(0, x - 1); //protect out of bounds
                int xx1 = x;
                int xx2 = std::min(x + 1, width - 1);

		// establish expected values for sobel
                float diff_x = -input(xx0, yy0) + input(xx2, yy0) +
                               -2 * input(xx0, yy1) + 2 * input(xx2, yy1) +
                               -input(xx0, yy2) + input(xx2, yy2);

                float diff_y = -input(xx0, yy0) + input(xx0, yy2) +
                               -2 * input(xx1, yy0) + 2 * input(xx1, yy2) +
                               -input(xx2, yy0) + input(xx2, yy2);

                expect[x][y] = static_cast<T>(sqrt(diff_x*diff_x + diff_y*diff_y));
            }
        }

        func(input, output);

	// check expect match output
        for (int y=1; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T actual = output(x, y);
                // Compiler differences for C++ and Halide cause
                // rounding errors for ints
                if (abs(expect[x][y] - actual) > 1) {
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

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(sobel_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(sobel_u16);
#endif
}

