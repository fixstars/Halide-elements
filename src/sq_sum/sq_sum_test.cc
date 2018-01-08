#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "Util.h"

#include "sq_sum_u8.h"
#include "sq_sum_u16.h"
#include "sq_sum_u32.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer0,  struct halide_buffer_t *_dst_buffer))
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
        auto output = mk_null_buffer<double>({1, 1});
        double actual_total;
        double expect_total = 0.0;
	
        func(input, output);
	actual_total = output(0, 0);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                expect_total += (double) input(x, y) * (double) input(x, y);
            }
        }
        if (expect_total != actual_total) {
            throw std::runtime_error(format("Error: expect_total = %f, actual_total = %f", expect_total, actual_total).c_str());
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Sucess!\n");
    return 0;
}

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(sq_sum_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(sq_sum_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(sq_sum_u32);
#endif
}
