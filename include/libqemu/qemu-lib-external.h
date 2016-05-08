#ifndef _QEMU_LIB_EXTERNAL_H
#define _QEMU_LIB_EXTERNAL_H

#include <llvm-c/Core.h>
#include <stdint.h>
#include <libqemu/qemu-lib.h>
#ifdef __cplusplus
extern "C" {
#endif 

/**
 * This union describes code flags for each architecture.
 * Code flags influence translation, e.g., the .arm.thumb flag
 * will tell the disassembler to disassemble as ARM Thumb instructions
 * (16 bit length) instead of ARM instructions (32 bit length).
 */
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

/* Taken from TCGMemOp in tcg/tcg.h. 
   Needs to be kept synchronous with the structure there.
   Constants for qemu_ld and qemu_st for the Memory Operation field.  */
typedef enum LibqemuMemOp {
    LQ_MO_8     = 0,
    LQ_MO_16    = 1,
    LQ_MO_32    = 2,
    LQ_MO_64    = 3,
    LQ_MO_SIZE  = 3,   /* Mask for the above.  */

    LQ_MO_SIGN  = 4,   /* Sign-extended, otherwise zero-extended.  */

    LQ_MO_BSWAP = 8,   /* Host reverse endian.  */

    /* MO_UNALN accesses are never checked for alignment.
       MO_ALIGN accesses will result in a call to the CPU's
       do_unaligned_access hook if the guest address is not aligned.
       The default depends on whether the target CPU defines ALIGNED_ONLY.  */
    LQ_MO_AMASK = 16,

    LQ_MO_CODE = 128
} LibqemuMemOp;

/**
 * Initialize the library.
 * @param ld_handler Handler function for memory read accesses.
 * @param st_handler Currently unused and ignored. 
 * Might be used later if emulation is added. 
 * @return 0 on success or error code.
 */ 
int libqemu_init(libqemu_load_handler *ld_handler, libqemu_store_handler *st_handler);
typedef int(*libqemu_init_fn)(libqemu_load_handler*, libqemu_store_handler);

/**
 * Get the LLVM module that the code is translated into.
 * @return Reference to LLVM module.
 */
LLVMModuleRef libqemu_get_module(void);
typedef LLVMModuleRef(*libqemu_get_module_fn)(void);

/**
 * Get the name of the target that this library was built for.
 * @return Libqemu target name.
 */
const char* libqemu_get_target_name(void);
typedef const char*(*libqemu_get_target_name_fn)(void);

/**
 * Generate LLVM intermediate code from machine code.
 * @param pc Instruction pointer where machine code starts.
 * @param flags Code flags that influence translation. See CodeFlags structure for detauls.
 * @param single_inst If set to <b>true</b> only a single instrucdtion will be translated,
 *    otherwise a basic block.
 * @param[out] Pointer to generated LLVM function containing the translation of a machine 
 *    instruction or BB will be stored in this value.
 * @return 0 on success, error code otherwise.
 */
int libqemu_gen_intermediate_code(uint64_t pc, CodeFlags flags, bool single_inst, LLVMValueRef * function);
typedef int(*libqemu_gen_intermediate_code_fn)(uint64_t, CodeFlags, bool, LLVMValueRef*);

/**
 * Raise an error which will abort IR translation.
 * This function is useful to call when nonexistent memory is referenced in a memory handler.
 * The libqemu_gen_intermediate_code function will then return immediatly with the error code.
 * @param env The env pointer which is passed to the memory load handler.
 * @param code Error code. Should not be 0.
 */
__attribute__((noreturn)) void libqemu_raise_error(void *env, int code);
typedef void(*libqemu_raise_error_fn)(void*, int);

int libqemu_get_pc_indices(unsigned indices[], size_t* num_indices);
typedef int(*libqemu_get_pc_indices_fn)(unsigned [], size_t*);

#ifdef __cplusplus
}
#endif

#endif /* _QEMU_LIB_EXTERNAL_H */
