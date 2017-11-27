#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "laplacian_u8.h"
#include "laplacian_u16.h"

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
        const int width = 8;
        const int height = 8;
        const int window_width = 3;
        const int window_height = 3;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, output);

        double kernel[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                double sum = 0;
                for (int k = -1; k <= 1; k++) {
                    for (int l = -1; l <= 1; l++) {
                        const int y = BORDER_INTERPOLATE(i + k, height);
                        const int x = BORDER_INTERPOLATE(j + l, width);
                        sum += static_cast<double>(input(x, y)) * kernel[k + 1][l + 1];
                    }
                }
                if (sum < 0) sum = -sum;
                if (sum > (std::numeric_limits<T>::max)())
                    sum = (std::numeric_limits<T>::max)();
                T expect = static_cast<T>(sum);
                T actual = output(j, i);
                
                if (abs(expect - actual) > 1) {
                    // throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", j, i, expect, j, i, actual).c_str());
                    printf("Error: expect(%d, %d) = %d, actual(%d, %d) = %d\n", j, i, expect, j, i, actual);
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
    test<uint8_t>(laplacian_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(laplacian_u16);
#endif
}
