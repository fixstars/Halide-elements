#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "erode_cross.h"

#include "testcommon.h"

using std::string;
using Halide::Runtime::Buffer;

int main()
{
    try {
        int ret = 0;
        
        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const int window_width = 4;
        const int window_height = 4;
        const int iteration = 2;
        Buffer<uint8_t> input(width, height);
        Buffer<uint8_t> output(width, height);
        uint8_t (*expect)[width][height], workbuf[2][width][height];
        std::srand(time(NULL));
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                input(x, y) = static_cast<uint8_t>(std::rand());
                workbuf[0][x][y] = input(x, y);
                output(x, y) = std::numeric_limits<uint8_t>::max();
            }
        }

        int k;
        for (int k; k<iteration; ++k) {
            for (int y=0; y<height; ++y) {
                for (int x=0; x<width; ++x) {
                    uint8_t minx = std::numeric_limits<uint8_t>::max(), miny = std::numeric_limits<uint8_t>::max();
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
        expect = &(workbuf[k%2]);
        
        erode_cross(input, window_width, window_height, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t actual = output(x, y);
                if ((*expect)[x][y] != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect[x][y], x, y, actual).c_str());
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
