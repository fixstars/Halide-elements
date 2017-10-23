#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "open_cross_u8.h"
#include "open_cross_u16.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, int32_t _window_width, int32_t _window_height, struct halide_buffer_t *_workbuf__1_buffer))
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

        int k;
        for (k=0; k<iteration; ++k) {
            for (int y=0; y<height; ++y) {
                for (int x=0; x<width; ++x) {
                    T minx = std::numeric_limits<T>::max(), miny = std::numeric_limits<T>::max();
                    for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                        int yy = y + j >= 0 ? y + j: 0;
                        yy = yy < height ? yy : height - 1;
                        if (miny > workbuf[k%2][x][yy]) {
                            miny = workbuf[k%2][x][yy];
                        }
                    }
                    for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
                        int xx = x + i >= 0 ? x + i: 0;
                        xx = xx < width ? xx : width - 1;
                        if (minx > workbuf[k%2][xx][y]) {
                            minx = workbuf[k%2][xx][y];
                        }
                    }
                    workbuf[(k+1)%2][x][y] = minx < miny ? minx : miny;
                }
            }
        }

        for (; k<2*iteration; ++k) {
            for (int y=0; y<height; ++y) {
                for (int x=0; x<width; ++x) {
                    T maxx = std::numeric_limits<T>::min(), maxy = std::numeric_limits<T>::min();
                    for (int j = -(window_height/2); j < -(window_height/2) + window_height; j++) {
                        int yy = y + j >= 0 ? y + j: 0;
                        yy = yy < height ? yy : height - 1;
                        if (maxy < workbuf[k%2][x][yy]) {
                            maxy = workbuf[k%2][x][yy];
                        }
                    }
                    for (int i = -(window_width/2); i < -(window_width/2) + window_width; i++) {
                        int xx = x + i >= 0 ? x + i: 0;
                        xx = xx < width ? xx : width - 1;
                        if (maxx < workbuf[k%2][xx][y]) {
                            maxx = workbuf[k%2][xx][y];
                        }
                    }
                    workbuf[(k+1)%2][x][y] = maxx > maxy ? maxx : maxy;
                }
            }
        }
        expect = &(workbuf[k%2]);

        func(input, window_width, window_height, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                T actual = output(x, y);
                if ((*expect)[x][y] != actual) {
                    printf("Error: expect(%d, %d) = %d, actual(%d, %d) = %d\n", x, y, (*expect)[x][y], x, y, actual);
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
    test<uint8_t>(open_cross_u8);
    test<uint16_t>(open_cross_u16);
}
