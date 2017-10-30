#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "affine.h"
#include "test_common.h"

using std::string;
using std::vector;

int main()
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 768;
        const int height = 1280;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<uint8_t>(extents);
        auto output = mk_null_buffer<uint8_t>(extents);

        float degrees = -30.0f;
        float scale_x = 0.3f;
        float scale_y = 0.4f;
        float shift_x = 100.0f;
        float shift_y = 200.0f;
        float skew_y = 30.0f;

        affine(input, degrees, scale_x, scale_y, shift_x, shift_y, skew_y, output);
        // operations are applied in the following order:
        //   1. scale about the origin
        //   2. shear in y direction
        //   3. rotation about the origin
        //   4. translation

        float cos_deg = cos(degrees * (float)M_PI / 180.0f);
        float sin_deg = sin(degrees * (float)M_PI / 180.0f);
        float tan_skew_y = tan(skew_y * (float)M_PI / 180.0f);
        float det = scale_x * scale_y;
        float a00 = scale_y * cos_deg;
        float a10 = scale_y * sin_deg;
        float a20 = - (a00 * shift_x + a10 * shift_y);
        float a01 = - scale_x * (sin_deg + cos_deg * tan_skew_y);
        float a11 =   scale_x * (cos_deg - sin_deg * tan_skew_y);
        float a21 = - (a01 * shift_x + a11 * shift_y);

        for (int y=shift_y; y<height; ++y) {
            for (int x=shift_x; x<width; ++x) {
                int tx = static_cast<int>((a00*x + a10*y + a20) / det);
                int ty = static_cast<int>((a01*x + a11*y + a21) / det);

                uint8_t expect = 255;
                if (tx >= 0 && tx < width && ty >= 0 && ty < height)
                    expect = input(tx, ty);
                uint8_t actual = output(x, y);
                if (expect != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                    x, y, static_cast<uint64_t>(expect),
                                                    x, y, static_cast<uint64_t>(actual)).c_str());
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
