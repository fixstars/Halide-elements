#include <math.h>

#include "run_common.h"
#include "xconvolution_hp_wrapper.h"

int main(int argc, char *argv[])
{
    XConvolution_hp_wrapper ins;
    int32_t width = 512;
    int32_t height = 512;
    int32_t ksz_max = 5;
    int32_t ksz = 3;
     
    dma_buffer_t ibuf = {.ptr=NULL, .size=width*height*sizeof(uint8_t),    .dim=0, .addr=0};
    dma_buffer_t kbuf = {.ptr=NULL, .size=ksz_max*ksz_max*sizeof(int16_t), .dim=0, .addr=0};
    dma_buffer_t obuf = {.ptr=NULL, .size=width*height*sizeof(uint8_t),    .dim=0, .addr=0};
    
    if (XConvolution_hp_wrapper_Initialize(&ins, "convolution_hp_wrapper") != XST_SUCCESS) {
        printf("Cannot initialize driver instance\n");
        goto finally;
    }
       
    pool_t pool;
    if (mempool_init(&pool)) goto finally;
    if (mempool_alloc(&pool, &ibuf)) goto finally;
    if (mempool_alloc(&pool, &obuf)) goto finally;
    if (mempool_alloc(&pool, &kbuf)) goto finally;

    for (int32_t y=0; y<height; ++y) {
        for (int32_t x=0; x<width; ++x) {
            ((uint8_t*)ibuf.ptr)[y * width + x] = 1;
            ((uint8_t*)obuf.ptr)[y * width + x] = 0;
        }
    }
   
    const uint32_t frac_bits = 10;
    for (int32_t y=0; y<ksz_max; ++y) {
        for (int32_t x=0; x<ksz_max; ++x) {
            if (x < ksz && y < ksz) {
                ((int16_t*)kbuf.ptr)[y * ksz_max + x] = (int16_t)round(1.0 * (1 << frac_bits));
            } else {
                ((int16_t*)kbuf.ptr)[y * ksz_max + x] = 0;
            }
        }
    }
       
    if (XConvolution_hp_wrapper_IsReady(&ins) != 1) {
        printf("Device is not ready\n");
        goto finally;
    };

    XConvolution_hp_wrapper_Set_p_in_port_addr_bv_V(&ins, ibuf.addr);
    XConvolution_hp_wrapper_Set_p_kernel_port_addr_bv_V(&ins, kbuf.addr);
    XConvolution_hp_wrapper_Set_p_kernel_size(&ins, (uint32_t)ksz);
    XConvolution_hp_wrapper_Set_p_out_port_addr_bv_V(&ins, obuf.addr);

    XConvolution_hp_wrapper_Start(&ins);
    
    while (XConvolution_hp_wrapper_IsDone(&ins) == 0) {
        usleep(10000); 
        puts("."); 
        fflush(stdout); 
    }
    
    for (int32_t y=0; y<height; ++y) {
        for (int32_t x=0; x<width; ++x) {
            uint8_t actual = ((uint8_t*)obuf.ptr)[y * width + x];
            uint8_t expect = 9;
            if (actual != expect) {
                printf("mismatched at [%d]: actual:%u, expect:%u\n", y*width+x, actual, expect);
                goto finally;
            } 
        }
    }
    
    printf("test passed\n");

finally:
    mempool_fini(&pool);
    XConvolution_hp_wrapper_Release(&ins);
    return 0;
}

