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

int libqemu_init(libqemu_load_handler *ld_handler, libqemu_store_handler *st_handler);
LLVMModuleRef libqemu_get_module(void);
LLVMValueRef libqemu_gen_intermediate_code(uint64_t pc, uint64_t flags, uint64_t cflags, bool single_inst);

#ifdef __cplusplus
}
#endif

#endif /* _QEMU_LIB_EXTERNAL_H */
