#include <math.h>

#include "run_common.h"
#include "xfft_hp_wrapper.h"

typedef struct {
  float x;
  float y;
} __attribute__((packed)) float2;

int verify(const float2 *src, const float2 *dst, const int n)
{
    float2 *ref_dst = (float2 *)malloc(n * sizeof(float2));
    assert(ref_dst != NULL);
   
    for (int i=0; i<n; ++i) {
        float2 d;
        d.x = 0;
        d.y = 0;
        for (int j=0; j<n; ++j) {
            float2 w;
            w.x = (float)cos(- 2 * M_PI * i * j / n);
            w.y = (float)sin(- 2 * M_PI * i * j / n);
            float2 s = src[j];
            d.x += s.x * w.x - s.y * w.y;
            d.y += s.x * w.y + s.y * w.x;
        }
        ref_dst[i] = d;
    }

    int result = 1;
    for (int i=0; i<n; ++i) {
        float2 expect = ref_dst[i];
        float2 actual = dst[i];
        if ((fabs(expect.x - actual.x) > 1e-3f) || (fabs(expect.y - actual.y) > 1e-3f)) {
            result = 0;
            printf("(%4d)=expect.x(%+7e), actual.x(%+7e), expect.y(%+7e), actual.y(%+7e)\n", 
                   i, expect.x, actual.x, expect.y, actual.y);
        }
    }

    free(ref_dst);

    return result;
}

int main(int argc, char *argv[])
{
    XFft_hp_wrapper ins;
    
    int32_t n = 256;
    int32_t batch_size = 4;
        
    dma_buffer_t ibuf = {.ptr=NULL, .size=n*batch_size*sizeof(float2), .dim=0, .addr=0};
    dma_buffer_t obuf = {.ptr=NULL, .size=n*batch_size*sizeof(float2), .dim=0, .addr=0};
       
    if (XFft_hp_wrapper_Initialize(&ins, "fft_hp_wrapper") != XST_SUCCESS) {
        printf("Cannot initialize driver instance\n");
        goto finally;
    }
   
    pool_t pool;
    if (mempool_init(&pool)) goto finally;
    if (mempool_alloc(&pool, &ibuf)) goto finally;
    if (mempool_alloc(&pool, &obuf)) goto finally;

    for (int32_t i=0; i<batch_size; ++i) {
        for (int32_t j=0; j<n; ++j) {
            ((float2*)ibuf.ptr)[i*n+j].x = (rand() % 100) * 1e-2f;
            ((float2*)ibuf.ptr)[i*n+j].y = (rand() % 100) * 1e-2f;
            ((float2*)obuf.ptr)[i*n+j].x = 0.0f;
            ((float2*)obuf.ptr)[i*n+j].y = 0.0f;
        }
    }
   
    XFft_hp_wrapper_Set_p_in_port_addr_bv_V(&ins, ibuf.addr);
    XFft_hp_wrapper_Set_p_out_port_addr_bv_V(&ins, obuf.addr);

    XFft_hp_wrapper_Start(&ins);
    
    while (XFft_hp_wrapper_IsDone(&ins) == 0) {
        usleep(10000); 
        putchar('.'); 
        fflush(stdout); 
    }

    for (int i=0; i<batch_size; ++i) {
        if (verify(&(((float2*)ibuf.ptr)[i*n]), &(((float2*)obuf.ptr)[i*n]), n) != 1) {
            printf("test failed\n");
            goto finally;
        }
    }

    printf("test passed\n");

finally:
    mempool_fini(&pool);
    XFft_hp_wrapper_Release(&ins);
    return 0;
}

