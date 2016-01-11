
#include "qemu-common.h"
#include <qemu.h>
#include <tcg/tcg.h>


/* exit the current TB from a signal handler. The host registers are
   restored in a state compatible with the CPU emulator
 */
void cpu_resume_from_signal(CPUState *cpu, void *puc)
{
/* From user-exec.c */
//#ifdef __linux__
//    struct ucontext *uc = puc;
//#elif defined(__OpenBSD__)
//    struct sigcontext *uc = puc;
//#endif
//
//    if (puc) {
//        /* XXX: use siglongjmp ? */
//#ifdef __linux__
//#ifdef __ia64
//        sigprocmask(SIG_SETMASK, (sigset_t *)&uc->uc_sigmask, NULL);
//#else
//        sigprocmask(SIG_SETMASK, &uc->uc_sigmask, NULL);
//#endif
//#elif defined(__OpenBSD__)
//        sigprocmask(SIG_SETMASK, &uc->sc_mask, NULL);
//#endif
//    }
    assert(false);
    cpu->exception_index = -1;
    siglongjmp(cpu->jmp_env, 1);
}


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
    qemu_mutex_lock(&tcg_ctx.tb_ctx.tb_lock);
    pthread_mutex_lock(&exclusive_lock);
    mmap_fork_start();
}

void fork_end(int child)
{
    mmap_fork_end(child);
    if (child) {
        CPUState *cpu, *next_cpu;
        /* Child processes created by fork() only have a single thread.
           Discard information about the parent threads.  */
        CPU_FOREACH_SAFE(cpu, next_cpu) {
            if (cpu != thread_cpu) {
                QTAILQ_REMOVE(&cpus, thread_cpu, node);
            }
        }
        pending_cpus = 0;
        pthread_mutex_init(&exclusive_lock, NULL);
        pthread_mutex_init(&cpu_list_mutex, NULL);
        pthread_cond_init(&exclusive_cond, NULL);
        pthread_cond_init(&exclusive_resume, NULL);
        qemu_mutex_init(&tcg_ctx.tb_ctx.tb_lock);
        gdbserver_fork(thread_cpu);
    } else {
        pthread_mutex_unlock(&exclusive_lock);
        qemu_mutex_unlock(&tcg_ctx.tb_ctx.tb_lock);
    }
}

/* Wait for pending exclusive operations to complete.  The exclusive lock
   must be held.  */
static inline void exclusive_idle(void)
{
    while (pending_cpus) {
        pthread_cond_wait(&exclusive_resume, &exclusive_lock);
    }
}

/* Start an exclusive operation.
   Must only be called from outside cpu_arm_exec.   */
static inline void start_exclusive(void)
{
    CPUState *other_cpu;

    pthread_mutex_lock(&exclusive_lock);
    exclusive_idle();

    pending_cpus = 1;
    /* Make all other cpus stop executing.  */
    CPU_FOREACH(other_cpu) {
        if (other_cpu->running) {
            pending_cpus++;
            cpu_exit(other_cpu);
        }
    }
    if (pending_cpus > 1) {
        pthread_cond_wait(&exclusive_cond, &exclusive_lock);
    }
}

/* Finish an exclusive operation.  */
static inline void __attribute__((unused)) end_exclusive(void)
{
    pending_cpus = 0;
    pthread_cond_broadcast(&exclusive_resume);
    pthread_mutex_unlock(&exclusive_lock);
}

/* Wait for exclusive ops to finish, and begin cpu execution.  */
static inline void cpu_exec_start(CPUState *cpu)
{
    pthread_mutex_lock(&exclusive_lock);
    exclusive_idle();
    cpu->running = true;
    pthread_mutex_unlock(&exclusive_lock);
}

/* Mark cpu as not executing, and release pending exclusive ops.  */
static inline void cpu_exec_end(CPUState *cpu)
{
    pthread_mutex_lock(&exclusive_lock);
    cpu->running = false;
    if (pending_cpus > 1) {
        pending_cpus--;
        if (pending_cpus == 1) {
            pthread_cond_signal(&exclusive_cond);
        }
    }
    exclusive_idle();
    pthread_mutex_unlock(&exclusive_lock);
}

void cpu_list_lock(void)
{
    pthread_mutex_lock(&cpu_list_mutex);
}

void cpu_list_unlock(void)
{
    pthread_mutex_unlock(&cpu_list_mutex);
}

/* do_brk() must return target values and target errnos. */
abi_long do_brk(abi_ulong new_brk)
{
	assert(false);
	return 0;
}
