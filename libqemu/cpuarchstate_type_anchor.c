#include <qemu-common.h>
#include <exec/cpu-all.h>

#ifdef TARGET_ARM
ARMCPU *cpu_type_anchor;
#else
#error Unknown arch!
#endif
