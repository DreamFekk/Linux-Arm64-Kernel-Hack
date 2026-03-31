#include <linux/slab.h>
#include <linux/random.h>
#include <linux/string.h>
typedef struct _COPY_MEMORY {
    pid_t pid;
    uintptr_t addr;
    void* buffer;
    size_t size;
} COPY_MEMORY, *PCOPY_MEMORY;
typedef struct _MODULE_BASE {
    pid_t pid;
    char* name;
    uintptr_t base;
} MODULE_BASE, *PMODULE_BASE;

