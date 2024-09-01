#include "defs.h"

struct vma {
    int free;
    struct vma *next;

    uint64 bg;      //begin of vm
    size_t length;
    int prot;
    int flags;

    struct file * file;
    off_t off;
};
