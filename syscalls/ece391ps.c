#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    if (0 != ece391_ps()) {
        ece391_fdputs (1, (uint8_t*)"Running ps command failed\n");
        return 3;
    }

    return 0;
}

