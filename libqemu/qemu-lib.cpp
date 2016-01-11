


extern "C" {
#include "qemu-common.h"
#include <exec/cpu-common.h>
#include <sysemu/cpus.h>
#include <qemu/main-loop.h>
#include <qemu/error-report.h>
#include <qemu.h>
#include "cpu.h"
#include <translate-all.h>
#include <tcg/tcg.h>
}
#include <libqemu/qemu-lib-external.h>
#include <llvm/IR/Function.h>
#include <libqemu/tcg-llvm.h>

int singlestep;
unsigned long mmap_min_addr;
THREAD CPUState *thread_cpu;
CPUArchState *env;
unsigned long reserved_va;
libqemu_load_handler *libqemu_ld = NULL;
libqemu_store_handler *libqemu_st = NULL;
static TCGLLVMContext *tcg_llvm_ctx = NULL;
unsigned long guest_base = 0;
int have_guest_base = 0;

int libqemu_init(libqemu_load_handler *ld_handler, libqemu_store_handler *st_handler)
{
    const char *cpu_model = (const char *) NULL;
    CPUState *cpu;
    
    libqemu_ld = ld_handler;
    libqemu_st = st_handler; 

    error_set_progname("libqemu");
    qemu_init_exec_dir("/tmp");

    module_call_init(MODULE_INIT_QOM);

#if defined(cpudef_setup)
    cpudef_setup(); /* parse cpu definitions in target config file (TBD) */
#endif
    if (cpu_model == NULL) {
#if defined(TARGET_I386)
#ifdef TARGET_X86_64
        cpu_model = "qemu64";
#else
        cpu_model = "qemu32";
#endif
#elif defined(TARGET_ARM)
        cpu_model = "any";
#elif defined(TARGET_UNICORE32)
        cpu_model = "any";
#elif defined(TARGET_M68K)
        cpu_model = "any";
#elif defined(TARGET_SPARC)
#ifdef TARGET_SPARC64
        cpu_model = "TI UltraSparc II";
#else
        cpu_model = "Fujitsu MB86904";
#endif
#elif defined(TARGET_MIPS)
#if defined(TARGET_ABI_MIPSN32) || defined(TARGET_ABI_MIPSN64)
        cpu_model = "5KEf";
#else
        cpu_model = "24Kf";
#endif
#elif defined TARGET_OPENRISC
        cpu_model = "or1200";
#elif defined(TARGET_PPC)
# ifdef TARGET_PPC64
        cpu_model = "POWER7";
# else
        cpu_model = "750";
# endif
#elif defined TARGET_SH4
        cpu_model = TYPE_SH7785_CPU;
#else
        cpu_model = "any";
#endif
    }
    tcg_exec_init(0);
    /* NOTE: we need to init the CPU at this stage to get
       qemu_host_page_size */
    cpu = cpu_init(cpu_model);
    if (!cpu) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(EXIT_FAILURE);
    }
    env = (CPUArchState *) cpu->env_ptr;
    cpu_reset(cpu);

    thread_cpu = cpu;

    /*
     * Now that page sizes are configured in cpu_init() we can do
     * proper page alignment for guest_base.
     */
//    guest_base = HOST_PAGE_ALIGN(guest_base);
//
//    if (reserved_va || have_guest_base) {
//        guest_base = init_guest_space(guest_base, reserved_va, 0,
//                                      have_guest_base);
//        if (guest_base == (unsigned long)-1) {
//            fprintf(stderr, "Unable to reserve 0x%lx bytes of virtual address "
//                    "space for use as guest address space (check your virtual "
//                    "memory ulimit setting or reserve less using -R option)\n",
//                    reserved_va);
//            exit(EXIT_FAILURE);
//        }
//
//        if (reserved_va) {
//            mmap_next_start = reserved_va;
//        }
//    }

    /* Now that we've loaded the binary, GUEST_BASE is fixed.  Delay
       generating the prologue until now so that the prologue can take
       the real value of GUEST_BASE into account.  */
    tcg_prologue_init(&tcg_ctx);
    tcg_llvm_ctx = tcg_llvm_initialize();
    return 0;
}

LLVMModuleRef libqemu_get_module(void)
{
    assert(false);
    return NULL;
}

LLVMValueRef libqemu_gen_intermediate_code(uint64_t pc, uint64_t cs_base, uint64_t flags, bool single_inst)
{
    TranslationBlock *tb;
    int max_cycles = CF_COUNT_MASK;
    bool ignore_icount = true;
    llvm::Function *function;

    singlestep = single_inst;

    /* Should never happen.
       We only end up here when an existing TB is too long.  */
    if (max_cycles > CF_COUNT_MASK)
        max_cycles = CF_COUNT_MASK;

    tb = tb_gen_code(thread_cpu, pc, cs_base, flags,
                     max_cycles | CF_NOCACHE
                         | (ignore_icount ? CF_IGNORE_ICOUNT : 0));
    tb->orig_tb = NULL;

    tcg_llvm_gen_code(tcg_llvm_ctx, &tcg_ctx, tb);
    function = static_cast<TCGPluginTBData *>(tb->opaque)->llvm_function;
    /* TODO: Generate LLVM here */
    tb_phys_invalidate(tb, -1);
    tb_free(tb);
    return NULL;
}
