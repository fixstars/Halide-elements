#ifndef RUN_COMMON_H
#define RUN_COMMON_H

#include <stdint.h>
#include <stdio.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    void *ptr;
    size_t size;
    int32_t dim;
    int32_t extent[4];
    uint32_t addr;
} buffer_t;

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
    unsigned char attr[1024];

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

static int mempool_alloc(pool_t *pool, buffer_t *buf)
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

#endif
