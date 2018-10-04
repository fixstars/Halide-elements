#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "flr.h"
#include "test_common.h"

using std::string;
using std::vector;

int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 9;
        const std::vector<int32_t> extents{width};
        auto input = mk_rand_buffer<float>(extents);
        auto output = mk_null_buffer<float>(extents);

        input(0) = -2.0f;
        input(1) = -1.5f;
        input(2) = -1.0f;
        input(3) = -0.5f;
        input(4) = 0.0f;
        input(5) = 0.5f;
        input(6) = 1.0f;
        input(7) = 1.5f;
        input(8) = 2.0f;

        int32_t val1 = (int32_t)(input(3) * 4096);
        printf("val1 = 0x%x\n", val1);
        printf("input(9) = %f\n", input(3));
        float _val1 = (val1 & ~0xfff) / 4096;
        printf("val1 & ~0xfff = 0x%x\n", val1 & ~0xfff);
        printf("_val1 = %f\n\n", _val1);
        
        int32_t val2 = (uint32_t)(input(7) * 4096);
        printf("val2 = 0x%x\n", val2);
        printf("input(1) = %f\n", input(7));
        float _val2 = (val2 & ~0xfff) / 4096;
        printf("val2 & ~0xfff = 0x%x\n", val2 & ~0xfff);
        printf("_val2 = %f\n\n", _val2);

        func(input, output);

        for (int x=0; x<width; ++x) {
            float expect = floor(input(x));
            float actual = output(x);
            if (expect != actual) {
                throw std::runtime_error(format("Error: expect(%d) = %f, actual(%d) = %f",
                                                x, expect,
                                                x, actual).c_str());
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
    test(flr);
}
