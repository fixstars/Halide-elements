#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "warp_perspective_bicubic_u8.h"
#include "warp_perspective_bicubic_u16.h"

#include "test_common.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

void getCubicKernel(float n, float w[4]){
    static const float a = -0.75f;
    w[0] = ((a*(n+1.0f)-5.0f*a)*(n+1.0f)+8.0f*a)*(n+1.0f)-4.0f*a;
    w[1] = ((a+2.0f)*n-(a+3.0f))*n*n+1.0f;
    w[2] = ((a+2.0f)*(1.0f-n)-(a+3.0f))*(1.0f-n)*(1.0f-n)+1.0f;
    w[3] = 1.0f-w[2]-w[1]-w[0];
}

template<typename T>
T interpolateBC(const Halide::Runtime::Buffer<T>& data, const int width, const int height,
                float x, float y, T border_value, const int border_type)
{
    if(x != x){x=0;}
    if(y != y){y=0;}
    x -= 0.5f;
    y -= 0.5f;

    int xf = static_cast<int>(x - 1.0f);
    int yf = static_cast<int>(y - 1.0f);

    xf = xf - (xf > x - 1);
    yf = yf - (yf > y - 1);

    float d[4][4];
    if (xf >= 0 && yf >= 0 && xf < width - 3 && yf < height - 3) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                d[i][j] = data(xf+j, yf+i);
            }
        }
    }else{
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (xf >= -j && yf >= -i && xf < width-j && yf < height-i) {
                    d[i][j] = data(xf+j, yf+i);
                } else if (border_type == 1) {
                    int xfj = BORDER_INTERPOLATE(xf + j, width);
                    int yfi = BORDER_INTERPOLATE(yf + i, height);
                    d[i][j] = data(xfj, yfi);
                } else {
                    assert(border_type == 0);
                    d[i][j] = border_value;
                }
            }
        }
    }

    float dx = (std::min)((std::max)(0.0f, x - xf - 1.0f), 1.0f);
    float dy = (std::min)((std::max)(0.0f, y - yf - 1.0f), 1.0f);

    float w[4];
    getCubicKernel(dx, w);

    float col[4];
    for (int i = 0; i < 4; i++) {
        col[i] = (d[i][0] * w[0] + d[i][1] * w[1])
                    + (d[i][2] * w[2] + d[i][3] * w[3]);
    }

    getCubicKernel(dy, w);
    float value = (col[0] * w[0] + col[1] * w[1])
                    + (col[2] * w[2] + col[3] * w[3]);

    T min = (std::numeric_limits<T>::min)();
    T max = (std::numeric_limits<T>::max)();
    return static_cast<T>(value < min ? min : value > max ? max: value + 0.5f);
}


template<typename T>
Halide::Runtime::Buffer<T>& NN_ref(Halide::Runtime::Buffer<T>& dst,
                                const Halide::Runtime::Buffer<T>& src,
                                const int32_t width, const int32_t height,
                                const T border_value, const int32_t border_type,
                                const Halide::Runtime::Buffer<double>& transform)
{
    /* avoid overflow from X-1 to X+2 */
    float imin = static_cast<float>((std::numeric_limits<int>::min)() + 1);
    float imax = static_cast<float>((std::numeric_limits<int>::max)() - 2);

    for(int i = 0; i < height; ++i){
        float org_y = static_cast<float>(i) + 0.5f;
        float src_xw0 = static_cast<float>(transform(2)) +
                        static_cast<float>(transform(1)) * org_y;
        float src_yw0 = static_cast<float>(transform(5)) +
                        static_cast<float>(transform(4)) * org_y;
        float src_w0 = static_cast<float>(transform(8)) +
                       static_cast<float>(transform(7)) * org_y;
        for(int j = 0; j < width; ++j){
            float org_x = static_cast<float>(j) + 0.5f;
            float inv_w = 1.0f / (src_w0 + static_cast<float>(transform(6)) * org_x);
            float src_x = (src_xw0 + static_cast<float>(transform(0)) * org_x) * inv_w;
            float src_y = (src_yw0 + static_cast<float>(transform(3)) * org_x) * inv_w;

            src_x = std::max(imin, std::min(imax, src_x));
            src_y = std::max(imin, std::min(imax, src_y));

            dst(j, i) = interpolateBC(src, width, height, src_x, src_y, border_value, border_type);
        }
    }

    return dst;
}


template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                     T _border_value, struct halide_buffer_t *_transform,
                     struct halide_buffer_t *_dst_buffer))

{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        const std::vector<int32_t> tableSize{9};
        const T border_value = mk_rand_scalar<T>();
        const int32_t border_type = 1; // 0 or 1
        auto transform = mk_rand_buffer<double>(tableSize);
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, border_value, transform, output);
        auto expect = mk_null_buffer<T>(extents);
        expect = NN_ref(expect, input, width, height, border_value, border_type, transform);

        //for each x and y
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                if (expect(x, y) != output(x, y)) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                    x, y, expect(x, y), x, y, output(x, y)).c_str());
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

int main(int argc, char **argv) {
#ifdef TYPE_u8
    test<uint8_t>(warp_perspective_bicubic_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(warp_perspective_bicubic_u16);
#endif

}
