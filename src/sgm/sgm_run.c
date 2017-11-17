#include "run_common.h"
#include "xsgm_hp_wrapper.h"

int main(int argc, char *argv[])
{
    XSgm_hp_wrapper ins;
    
    int32_t width = 641;
    int32_t height = 555;
    
    dma_buffer_t lbuf = {.ptr=NULL, .size=width*height*sizeof(uint8_t), .dim=0, .addr=0};
    dma_buffer_t rbuf = {.ptr=NULL, .size=width*height*sizeof(uint8_t), .dim=0, .addr=0};
    dma_buffer_t obuf = {.ptr=NULL, .size=width*height*sizeof(uint8_t), .dim=0, .addr=0};
 
    uint8_t *disp = NULL;
  
    if (XSgm_hp_wrapper_Initialize(&ins, "sgm_hp_wrapper") != XST_SUCCESS) {
        printf("Cannot initialize driver instance\n");
        goto finally;
    }
     
    pool_t pool;
    if (mempool_init(&pool)) goto finally;
    if (mempool_alloc(&pool, &lbuf)) goto finally;
    if (mempool_alloc(&pool, &rbuf)) goto finally;
    if (mempool_alloc(&pool, &obuf)) goto finally;

    if (load_pgm("data/left.pgm", lbuf.ptr, width, height)) {
        printf("Cannot load left.pgm\n");
        goto finally;
    }
    if (load_pgm("data/right.pgm", rbuf.ptr, width, height)) {
        printf("Cannot load right.pgm\n");
        goto finally;
    }

    memset(obuf.ptr, 0, obuf.size);

    XSgm_hp_wrapper_Set_p_in_l_port_addr_bv_V(&ins, lbuf.addr);
    XSgm_hp_wrapper_Set_p_in_r_port_addr_bv_V(&ins, rbuf.addr);
    XSgm_hp_wrapper_Set_p_out_port_addr_bv_V(&ins, obuf.addr);

    XSgm_hp_wrapper_Start(&ins);
    
    while (XSgm_hp_wrapper_IsDone(&ins) == 0) {
        usleep(10000); 
        putchar('.'); 
        fflush(stdout); 
    }
    
    save_pgm("out.pgm", obuf.ptr, width, height);

    printf("test passed\n");
finally:
    if (disp) free(disp);
    mempool_fini(&pool);
    XSgm_hp_wrapper_Release(&ins);
    return 0;
}

