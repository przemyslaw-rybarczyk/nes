#include <stdlib.h>

#include "global.h"

u8 controller_state = 0x00;

void *s_malloc(size_t size) {
    void *p = malloc(size);
    if (p == NULL) {
        eprintln("Error: could not allocate memory");
        exit(1);
    }
    return p;
}
