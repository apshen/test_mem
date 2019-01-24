#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static size_t string_to_int(const char *str)
{
    size_t ret = 0;
    if(strncmp("0x", str, 2) == 0)
        ret = strtoul(str, 0, 16);
    else
        ret = strtoul(str, 0, 10);

    return ret;
}


int main(int argc, char *argv[])
{
    size_t mem_size = 0x100000000ul;
    size_t mem_offset = 0x6000000000ul;

    int use_virt_mem = 0;
    if(argc > 1)
    {
        if(strcmp(argv[1], "virt") == 0)
            use_virt_mem = 1;
	else 
        {
            mem_offset = string_to_int(argv[1]);
            if(errno != 0)
            {
                fprintf(stderr, "can't read memory offset from string '%s': '%s' %d\n", argv[1], strerror(errno), errno);
                return 1;
            }
        }
    }
    if(argc > 2)
    {
        mem_size = string_to_int(argv[2]);
        if(errno != 0)
        {
            fprintf(stderr, "can't read memory size from string '%s': '%s' %d\n", argv[2], strerror(errno), errno);
            return 1;
        }
    }


    volatile void *p = 0;

    if(use_virt_mem)
    {
        p = malloc(mem_size);
        printf("allocated %zu bytes\n", mem_size);
        if(!p)
        {
            fprintf(stderr, "can't alloc %zu bytes: '%s' %d\n", mem_size, strerror(errno), errno);
            return 1;
        }
    }
    else
    {
        // open /dev/mem
        int fd = open("/dev/mem", O_RDWR | O_SYNC);
        if(fd == -1)
        {
            fprintf(stderr, "can't open /dev/mem: '%s' %d\n", strerror(errno), errno);
            return 1;
        }
        printf("opened /dev/mem\n");

        // mmap /dev/mem
        p = mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mem_offset);
        if(!p)
        {
            fprintf(stderr, "can't mmap /dev/mem: '%s' %d\n", strerror(errno), errno);
            return 1;
        }

        printf("mmaped %zu bytes of /dev/mem at %lx\n", mem_size, mem_offset);
    }


    //test mem

    srand(time(0));
    size_t seed = rand();
    
    printf("seed: %zu\n", seed);
    printf("writing memory\n");
    printf("\r...0%%");
    fflush(stdout);

    size_t percent = 0;

    volatile uint32_t *mem = (volatile uint32_t *)(p);
    for(size_t i = 0; i < mem_size / sizeof(uint32_t); ++i)
    {
        if((i * 100 * sizeof(uint32_t)) / mem_size > percent)
        {
            percent = (i * 100 * sizeof(uint32_t)) / mem_size;
            printf("\r...%zu%%", percent);
	    fflush(stdout);
	}

        mem[i] = (uint32_t)(seed * i);
    }

    printf("\r...100%%\n");
    printf("reading memory\n");
    printf("\r...0%%");
    fflush(stdout);

    percent = 0;

    volatile uint32_t val = 0;
    for(size_t i = 0; i < mem_size / sizeof(uint32_t); ++i)
    {
        if((i * 100 * sizeof(uint32_t)) / mem_size > percent)
        {
            printf("\r...%zu%%", percent);
	    fflush(stdout);
            percent = (i * 100 * sizeof(uint32_t)) / mem_size;
	}

        val = mem[i];
        if(val != (uint32_t)(seed * i))
        {
            fprintf(stderr, "\nmem test failed at %zu; expected: %x; read: %x\n", i, (uint32_t)(seed * i), val);
            return 1;
        }
    }

    printf("\r...100%%\n");
    printf("test passed\n");
    return 0;
}
