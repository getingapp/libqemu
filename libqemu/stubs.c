
#include "qemu-common.h"
#include <qemu.h>
#include <tcg/tcg.h>


int do_strace;
const char *qemu_uname_release;


/***********************************************************/
/* Helper routines for implementing atomic operations.  */

/* To implement exclusive operations we force all cpus to syncronise.
   We don't require a full sync, only that no cpus are executing guest code.
   The alternative is to map target atomic ops onto host equivalents,
   which requires quite a lot of per host/target work.  */
static pthread_mutex_t cpu_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t exclusive_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t exclusive_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t exclusive_resume = PTHREAD_COND_INITIALIZER;
static int pending_cpus;

/* Make sure everything is in a consistent state for calling fork().  */
void fork_start(void)
{
}

void fork_end(int child)
{
}

/* Wait for pending exclusive operations to complete.  The exclusive lock
   must be held.  */
static inline void exclusive_idle(void)
{
}

/* Start an exclusive operation.
   Must only be called from outside cpu_arm_exec.   */
static inline void start_exclusive(void)
{
}

/* Finish an exclusive operation.  */
static inline void __attribute__((unused)) end_exclusive(void)
{
}

/* Wait for exclusive ops to finish, and begin cpu execution.  */
static inline void cpu_exec_start(CPUState *cpu)
{
}

/* Mark cpu as not executing, and release pending exclusive ops.  */
static inline void cpu_exec_end(CPUState *cpu)
{
}

void cpu_list_lock(void)
{
}

void cpu_list_unlock(void)
{
}

#ifdef TARGET_I386
/***********************************************************/
/* CPUX86 core interface */

uint64_t cpu_get_tsc(CPUX86State *env)
{
        return 0;
}

static void write_dt(void *ptr, unsigned long addr, unsigned long limit,
                     int flags)
{
}

static uint64_t *idt_table;
#ifdef TARGET_X86_64
static void set_gate64(void *ptr, unsigned int type, unsigned int dpl,
                       uint64_t addr, unsigned int sel)
{
}
/* only dpl matters as we do only user space emulation */
static void set_idt(int n, unsigned int dpl)
{
}
#else
static void set_gate(void *ptr, unsigned int type, unsigned int dpl,
                     uint32_t addr, unsigned int sel)
{
}

/* only dpl matters as we do only user space emulation */
static void set_idt(int n, unsigned int dpl)
{
}
#endif
#endif

void cpu_loop(CPUArchState *env)
{
}

void task_settid(TaskState *ts) {}
void init_task_state(TaskState *ts) {}
void
print_syscall(int num,
                              abi_long arg1, abi_long arg2, abi_long arg3,
                                            abi_long arg4, abi_long arg5, abi_long arg6) {}
#if defined(TARGET_I386)
int cpu_get_pic_interrupt(CPUX86State *env) {return 0;}
#endif

CPUArchState *cpu_copy(CPUArchState *env) {return NULL;}
void stop_all_tasks(void) {}
void print_syscall_ret(int num, abi_long ret) {}
void gemu_log(const char *fmt, ...) {}
        
