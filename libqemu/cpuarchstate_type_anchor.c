#include <qemu-common.h>
#include <exec/cpu-all.h>

#ifdef TARGET_ARM
ARMCPU *cpu_type_anchor;
#elif TARGET_I386
X86CPU *cpu_type_anchor;
#elif TARGET_X86_64
X86CPU *cpu_type_anchor;
#else
#error Unknown arch!
#endif
