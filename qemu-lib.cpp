#include <libqemu/qemu-lib.h>

extern "C" {
int singlestep = 0;
}

int libqemu_init(void)
{
    return 0;
}
