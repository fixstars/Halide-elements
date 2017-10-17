#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "float_err_u8.h"

#include "test_common.h"

int main()
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1;
        const int window_width = 2;
        const float sigma = 1.0;
        const std::vector<int32_t> extents{width+1};
        const std::vector<int32_t> extents2{width};
        auto input = mk_null_buffer<uint8_t>(extents);
        auto filter = mk_null_buffer<float>(extents);
        auto output = mk_null_buffer<float>(extents2);
        uint32_t val0, val1;

        filter(0) = expf(0);
        filter(1) = expf(-0.5);
        
        input(0) = 100;
        input(1) = 100;

        float_err_u8(input, width, filter, window_width, output);

        float expect_f = 0.0f;
        for (int i = 0; i < window_width; i++) {
            expect_f += filter(i) * input(i);
        }
        float actual_f = output(0);
        memcpy((void *)&val0, (void *)&expect_f, 4);
        memcpy((void *)&val1, (void *)&actual_f, 4);
        printf("expect_f = %x, actual_f = %x\n", val0, val1);
        if (expect_f != actual_f) {
            throw std::runtime_error(format("Error: expect = %f, actual = %f", expect_f, actual_f).c_str());
        }
            
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}
