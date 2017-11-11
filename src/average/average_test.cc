#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "average_u8.h"

#include "test_common.h"

int main()
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 32;
        const int height = 24;
        const uint8_t add_val = 128;
        const std::vector<int32_t> extents{width, height};
        auto input0 = mk_rand_buffer<uint8_t>(extents);
 
        auto output = mk_null_buffer<uint8_t>(extents);

        average_u8(input0, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint32_t expect;
                int ax, ay;
                expect = 0;
                for (int wx = -1; wx < 2; wx++) {
                    for (int wy = -1; wy < 2; wy++) {
                        ax = x + wx;
                        ay = y + wy;

                        if (ax < 0) ax = 0;
                        if (ay < 0) ay = 0;
                        if (width <= ax) ax = width - 1;
                        if (height <= ay) ay = height - 1;
                        
                        expect += static_cast<uint32_t>(input0(ax, ay));
                        //std::cout << "ax=" << ax << ":ay=" << ay << ":input=" << static_cast<uint32_t>(input0(ax, ay)) << ":exp=" << expect << std::endl;
                    }
                }
                expect = static_cast<uint32_t>(roundf(static_cast<float>(expect) / 9.0f));


                
                uint8_t actual = output(x, y);
                
                if (expect != actual) {

                    std::cout << "org:" ;
                    
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
