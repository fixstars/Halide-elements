#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "close_rect_u8.h"
#include "close_rect_u16.h"

#include "test_common.h"


// returns index of result workbuf
template<typename T>
int conv_rect(int width, int height, int window_width, int window_height, int iteration,
              T* workbuf, const T&(*f)(const T&, const T&), T init, int k) {

    int itr;
    for (itr=k; itr<k+iteration; ++itr) {
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T min = init;
                for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                    int yy = y + j >= 0 ? y + j: 0;
                    yy = yy < height ? yy : height - 1;
                    for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
                        int xx = x + i >= 0 ? x + i: 0;
                        xx = xx < width ? xx : width - 1;
                        min = f(min, workbuf[itr%2*width*height + xx*height + yy]);
                    }
                }
                workbuf[((itr+1)%2)*width*height + x*height + y] = min;
            }
        }
    }
    return itr;
}


template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_workbuf__1_buffer))
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
		const int iteration = 2;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);
        T (*expect)[width][height], workbuf[2][width][height];

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                workbuf[0][x][y] = input(x, y);
            }
        }
        
        // dilate
        int k = conv_rect(width, height, window_width, window_height, iteration,
                                &workbuf[0][0][0], static_cast<const T&(*)(const T&, const T&)>(std::max), std::numeric_limits<T>::min(), k);

        // erode
        k = conv_rect(width, height, window_width, window_height, iteration,
                                    &workbuf[0][0][0], static_cast<const T&(*)(const T&, const T&)>(std::min), std::numeric_limits<T>::max(), 0);
        expect = &(workbuf[k%2]);

        func(input, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T actual = output(x, y);
                if ((*expect)[x][y] != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, (*expect)[x][y], x, y, actual).c_str());
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
    test<uint8_t>(close_rect_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(close_rect_u16);
#endif
}
