#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>
#include <iomanip>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "sin_cos.h"

#include "test_common.h"

int test(int (*func)(struct halide_buffer_t *_src_buffer1, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_real_buffer<float>(extents, 0, 1);
        auto output = mk_null_buffer<float>(extents);

        func(input, output);

        double diff_max = 0.0;
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                float expect = (x + y) % 2 == 0 ? sin(input(x, y)) : cos(input(x, y));
                float actual = output(x, y);
                double diff = abs(expect - actual);
                diff_max = std::max(diff, diff_max);
                if (abs(expect - actual) > 0.000001) {
                    throw std::runtime_error(format("Error: expect: sin(%f) = %f, actual: sin(%f) = %f", input(x, y), expect, input(x, y), actual).c_str());
                }
            }

        }
        std::cout <<  "Max diff = " << std::setprecision(17) << diff_max << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}

int main()
{
    test(sin_cos);
}
