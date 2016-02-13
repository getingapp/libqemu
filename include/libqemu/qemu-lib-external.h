#ifndef _QEMU_LIB_EXTERNAL_H
#define _QEMU_LIB_EXTERNAL_H

#include <llvm-c/Core.h>
#include <stdint.h>
#include <libqemu/qemu-lib.h>
#ifdef __cplusplus
extern "C" {
#endif 

#define FLAG_NONE 0
#define FLAG_ARM_THUMB      (1 << 0)
#define FLAG_ARM_BSWAP_CODE (1 << 16)

#define CFLAG_NONE 0

typedef union CodeFlags
{
    struct {
        unsigned thumb          : 1; /* Disassemble as Thumb instructions */
        unsigned veclen         : 3;
        unsigned vecstride      : 3;
        unsigned vfpen          : 1;
        unsigned condexec       : 8;
        unsigned bswap_code     : 1; /* Byteswap code before disassembling */
        unsigned xscale_cpar    : 2;
        unsigned ns             : 1;
    } arm;
    struct {
        unsigned cpl            : 2;
        unsigned softmmu        : 1;
        unsigned inhibit_irq    : 1;
        unsigned cs32           : 1;
        unsigned ss32           : 1;
        unsigned addseg         : 1; 
        unsigned pe             : 1;
        unsigned tf             : 1;
        unsigned mp             : 1;
        unsigned em             : 1;
        unsigned ts             : 1;
        unsigned iopl           : 2;
        unsigned lma            : 1;
        unsigned cs64           : 1;
        unsigned rf             : 1;
        unsigned vm             : 1;
        unsigned ac             : 1;
        unsigned smm            : 1;
        unsigned svme           : 1;
        unsigned svmi           : 1;
        unsigned osfxsr         : 1;
        unsigned smap           : 1;
        unsigned iobpt          : 1;
    } x86;
    uint64_t _value;
} CodeFlags;

int libqemu_init(libqemu_load_handler *ld_handler, libqemu_store_handler *st_handler);
LLVMModuleRef libqemu_get_module(void);
LLVMValueRef libqemu_gen_intermediate_code(uint64_t pc, CodeFlags flags, bool single_inst);
__attribute__((noreturn)) void libqemu_raise_error(void *env, int code);

#ifdef __cplusplus
}
#endif

#endif /* _QEMU_LIB_EXTERNAL_H */
