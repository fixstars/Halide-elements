#ifndef RUN_COMMON_H
#define RUN_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

typedef struct {
    void *ptr;
    size_t size;
    int32_t dim;
    int32_t extent[4];
    uint32_t addr;
} dma_buffer_t;

typedef struct {
    int fd;
    uint8_t *ptr;
    uint32_t size;
    uint32_t phys_addr;
    uint32_t offset;
} pool_t;

static int mempool_init(pool_t *pool)
{
    int fd = 0;
    char attr[1024];

    if ((fd  = open("/dev/udmabuf0", O_RDWR | O_SYNC)) == -1) {
        printf("cannot open udmabuf0\n");
        return 1;
    }

    uint32_t size = 0;
    {
        int fd = 0;
        if ((fd  = open("/sys/class/udmabuf/udmabuf0/size", O_RDONLY)) == -1) {
            printf("cannot get size\n");
            return 1;
        }
        read(fd, attr, 1024);
        sscanf(attr, "%u", &size);
        close(fd);
    }


    uint32_t phys_addr = 0;
    {
        int fd = 0;
        if ((fd  = open("/sys/class/udmabuf/udmabuf0/phys_addr", O_RDONLY)) == -1) {
            printf("cannot get phys_addr\n");
            return 1;
        }
        read(fd, attr, 1024);
        sscanf(attr, "%x", &phys_addr);
        close(fd);
    }

    uint8_t *ptr = (uint8_t*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == NULL) {
        printf("cannot mmap physically-backed mempool\n");
        return 1;
    }

    pool->fd = fd;
    pool->ptr = ptr;
    pool->size = size;
    pool->phys_addr = phys_addr;
    pool->offset = 0;

    return 0;
}

static int mempool_alloc(pool_t *pool, dma_buffer_t *buf)
{
    // Align to 4KiB
    size_t size = (buf->size + 4096 - 1) / 4096 * 4096;

    if (pool->offset + size > pool->size) {
        printf("Memory pool is insufficient\n");
        return 1;
    }

    buf->ptr = pool->ptr + pool->offset;
    buf->addr = pool->phys_addr + pool->offset;

    pool->offset += size;

    return 0;
}

static void mempool_fini(pool_t *pool)
{
    if (pool->ptr != NULL) {
        munmap(pool->ptr, pool->size);
        pool->ptr = NULL;
    }

    if (pool->fd) {
        pool->fd = 0;
        close(pool->fd);
    }
}

int load_pgm(const char *fname, uint8_t *buffer, int32_t width, int32_t height)
{
    FILE *fd = fopen(fname, "rb");
    if (fd == NULL) {
        printf("Invalid path\n");
        return 1;
    }

    char header[256];
    fscanf(fd, "%s\n", header);
    int32_t w=0, h=0;
    fscanf(fd, "%d %d\n", &w, &h);
    if (w != width || h != height) {
        printf("Mismatched size\n");
        return 1;
    }
    int32_t max_depth;
    fscanf(fd, "%d\n", &max_depth);

    uint8_t *buf = (uint8_t*)malloc(w*h*sizeof(uint8_t));
    if (buf == NULL) {
        printf("Cannot allocate buffer\n");
        fclose(fd);
        return 1;
    }
    fread(buf, sizeof(uint8_t), w*h, fd);
    memcpy(buffer, buf, w*h*sizeof(uint8_t));
    free(buf);

    fclose(fd);
    return 0;
}

int save_pgm(const char *fname, const uint8_t *buffer, int32_t width, int32_t height)
{
    FILE *fd = fopen(fname, "wb");
    if (fd == NULL) {
        printf("Invalid path\n");
        return 1;
    }

    fprintf(fd, "P5\n");
    fprintf(fd, "%d %d\n", width, height);
    fprintf(fd, "255\n");

    uint8_t *buf = (uint8_t*)malloc(width*height*sizeof(uint8_t));
    memcpy(buf, buffer, width*height*sizeof(uint8_t));
    fwrite(buf, sizeof(uint8_t), width*height, fd);
    free(buf);

    fclose(fd);
    return 0;
}

int save_ppm(const char *fname, const uint8_t *buffer, int32_t channel, int32_t width, int32_t height)
{
    if (channel != 3 && channel != 4) {
        printf("Invalid format\n");
        return 1;
    }

    FILE *fd = fopen(fname, "wb");
    if (fd == NULL) {
        printf("Invalid path\n");
        return 1;
    }

    fprintf(fd, "P6\n");
    fprintf(fd, "%d %d\n", width, height);
    fprintf(fd, "255\n");

    uint8_t *buf = (uint8_t*)malloc(3*width*height);
    for (int32_t y=0; y<height; ++y) {
        for (int32_t x=0; x<width; ++x) {
            for (int32_t c=0; c<3; ++c) {
                buf[y*3*width+x*3+c] = buffer[y*channel*width+x*channel+c];
            }
        }
    }
    fwrite(buf, sizeof(uint8_t), 3*width*height, fd);
    free(buf);

    fclose(fd);
    return 0;
}

} //anonymous namespace
#endif
