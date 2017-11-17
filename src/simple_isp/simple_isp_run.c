#include "run_common.h"
#include "xsimple_isp_hp_wrapper.h"

void fill_bayer_pattern(dma_buffer_t *buf, int width, int height)
{
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            if (x < width/2) {
                if (y < height/2) {
                    if (x%2 == 0 && y%2 == 0) {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0x03FF;
                    } else {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0;
                    }
                } else {
                    if ((x%2 == 0 && y%2 == 1) || (x%2 == 1 && y%2 == 0)) {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0x03FF;
                    } else {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0;
                    }
                }
            } else {
                if (y < height/2) {
                    if ((x%2 == 0 && y%2 == 1) || (x%2 == 1 && y%2 == 0)) {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0x03FF;
                    } else {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0;
                    }
                } else {
                    if (x%2 == 1 && y%2 == 1) {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0x03FF;
                    } else {
                        ((uint16_t*)buf->ptr)[y*width+x] = 0;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    XSimple_isp_hp_wrapper ins;
    int32_t width = 3280;
    int32_t height = 2486;
    int32_t channel = 4;
    
    dma_buffer_t ibuf = {.ptr=NULL, .size=width*height*sizeof(uint16_t),        .dim=0, .addr=0};
    dma_buffer_t obuf = {.ptr=NULL, .size=channel*width*height*sizeof(uint8_t), .dim=0, .addr=0};

    const uint16_t optical_black_clamp_value = 16;
    const float gamma_value = 1.0f/1.8f;
    const float saturation_value = 0.6f;
    uint32_t reg_data = 0;

    if (argc == 4) {
        sscanf(argv[1], "%d", &optical_black_clamp_value);
        sscanf(argv[2], "%f", &gamma_value);
        sscanf(argv[3], "%f", &saturation_value);
    }

    if (XSimple_isp_hp_wrapper_Initialize(&ins, "simple_isp_hp_wrapper") != XST_SUCCESS) {
        printf("Cannot initialize driver instance\n");
        goto finally;
    }
       
    pool_t pool;
    if (mempool_init(&pool)) goto finally;
    if (mempool_alloc(&pool, &ibuf)) goto finally;
    if (mempool_alloc(&pool, &obuf)) goto finally;

    fill_bayer_pattern(&ibuf, width, height);
    memset(obuf.ptr, 0, obuf.size);

    XSimple_isp_hp_wrapper_Set_p_in_port_addr_bv_V(&ins, ibuf.addr);
    XSimple_isp_hp_wrapper_Set_p_out_port_addr_bv_V(&ins, obuf.addr);
    memcpy(&reg_data, &saturation_value, sizeof(float));
    XSimple_isp_hp_wrapper_Set_p_saturation_value(&ins, reg_data);
    memcpy(&reg_data, &optical_black_clamp_value, sizeof(uint16_t));
    XSimple_isp_hp_wrapper_Set_p_optical_black_value(&ins, reg_data);
    memcpy(&reg_data, &gamma_value, sizeof(float));
    XSimple_isp_hp_wrapper_Set_p_gamma_value(&ins, reg_data);

    XSimple_isp_hp_wrapper_Start(&ins);
    
    while (XSimple_isp_hp_wrapper_IsDone(&ins) == 0) {
        usleep(10000); 
        puts("."); 
        fflush(stdout); 
    }

    save_ppm("out.ppm", (const uint8_t*)obuf.ptr, channel, width, height);
    
    printf("test passed\n");

finally:
    mempool_fini(&pool);
    XSimple_isp_hp_wrapper_Release(&ins);
    return 0;
}
