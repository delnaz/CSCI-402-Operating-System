#include "types.h"
#include "globals.h"
#include "kernel.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "errno.h"

extern void *testproc(int arg1, void *arg2);
extern void *sunghan_deadlock_test(int arg1, void *arg2);
extern void *sunghan_test(int arg1, void *arg2);

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void       hard_shutdown(void);
static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);

static context_t bootstrap_context;
static int gdb_wait = GDBWAIT;


void *my_waitpid_test(int arg1, void *arg2) {
    do_exit(arg1);
    dbg(DBG_PRINT,"\nLeaving my_waitpid_test\n");
    return NULL;
}


void *edgeDoWaitpidTest(int arg1, void* arg2)
{
	pid_t pid = 0, ret_pid;
    	int retcode = 0;

	proc_t *testp = proc_create("TEST_PROC");
	kthread_t *testt = kthread_create(testp, my_waitpid_test, 0, NULL);
	ret_pid = do_waitpid(pid, 0, &retcode);
	dbg(DBG_PRINT,"\nreturn pid = %d\n", ret_pid);
	return NULL;
}

kmutex_t mtx1,mtx2;

void* mutexDeadlockOne (int arg1, void *arg2)
{
   kmutex_lock(&mtx1);
   dbg(DBG_PRINT,"\n Acquired mutex 1 \n");
   sched_make_runnable(curthr);
   sched_switch();
   kmutex_lock(&mtx2);
  
   return NULL;
}

void* mutexDeadlockTwo (int arg1, void *arg2)
{
   kmutex_lock(&mtx2);
   dbg(DBG_PRINT,"\n Acquired mutex 2 \n");
   sched_make_runnable(curthr);
   sched_switch();
   kmutex_lock(&mtx1);

   return NULL;
}


void *mutexDeadlockTest(int arg1, void *arg2)
{
   int status = 0;
   proc_t *proc1,*proc2 = NULL;
   kthread_t *thr1,*thr2 = NULL;
   kmutex_init(&mtx1);
   kmutex_init(&mtx2);

   proc1 = proc_create("proc1");
   dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc1->p_comm, proc1->p_pid);
   thr1 = kthread_create(proc1,mutexDeadlockOne, 0, NULL);
   
   proc2 = proc_create("proc2");
   dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc2->p_comm, proc2->p_pid);
   thr2 = kthread_create(proc2,mutexDeadlockTwo, 0, NULL);

   sched_make_runnable(thr1);
   sched_make_runnable(thr2);

   while(do_waitpid(-1,0,&status) > 0);
   
   return NULL;  

}


void *mutexlckTwice(int arg1, void *arg2)
{
   kmutex_lock(&mtx1);
   dbg(DBG_PRINT,"\n Acquired mutex 1 \n");
   dbg(DBG_PRINT,"\n Try acquiring mutex 1 again\n");
   kmutex_lock(&mtx1);

   return NULL;

}

void *tryMutexLockTwice(int arg1, void *arg2)
{

   int status = 0;
   proc_t *proc1 = NULL;
   kthread_t *thr1 = NULL;
   kmutex_init(&mtx1);

   proc1 = proc_create("proc1");
   dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc1->p_comm, proc1->p_pid);
   thr1 = kthread_create(proc1,mutexlckTwice, 0, NULL);
   
   sched_make_runnable(thr1);

   while(do_waitpid(-1,0,&status) > 0);
   
   return NULL;  

}

void *mutexthr1(int arg1, void *arg2)
{
   kmutex_lock(&mtx1);
   dbg(DBG_PRINT,"\n Acquired mutex 1 \n");
   sched_make_runnable(curthr);
   sched_switch();
   kmutex_unlock(&mtx1);

   return NULL;
}

void *mutexthr2(int arg1, void *arg2)
{
   dbg(DBG_PRINT,"\n Try to unlock mutex 1 \n");
   kmutex_unlock(&mtx1);
   return NULL;

}

void *threadUnlocksMutex(int arg1, void *arg2)
{

   int status = 0;
   proc_t *proc1,*proc2 = NULL;
   kthread_t *thr1,*thr2 = NULL;
   kmutex_init(&mtx1);
   kmutex_init(&mtx2);

   proc1 = proc_create("proc1");
   dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc1->p_comm, proc1->p_pid);
   thr1 = kthread_create(proc1,mutexthr1, 0, NULL);
   
   proc2 = proc_create("proc2");
   dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc2->p_comm, proc2->p_pid);
   thr2 = kthread_create(proc2,mutexthr2, 0, NULL);

   sched_make_runnable(thr1);
   sched_make_runnable(thr2);

   while(do_waitpid(-1,0,&status) > 0);
   
   return NULL;  

}

void *printDgbStatForThread(int arg1, void *arg2)
{

    dbg(DBG_PRINT,"\n Process %d running \n",arg1);
    return NULL;
}

void *createNkillReverse(int arg1, void *arg2)
{
   proc_t *proc[5];
   kthread_t *thr[5];
   int i = 0;

   for(i = 0 ; i < 5; i++)
   {
     proc[i] = proc_create("proc1");
     dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc[i]->p_comm, proc[i]->p_pid);
     thr[i] = kthread_create(proc[i],printDgbStatForThread, proc[i]->p_pid, NULL); 
     sched_make_runnable(thr[i]);    
   }
   
   for(i = 4 ; i > 0; i--)
   {
      /* Kill the process */
      dbg(DBG_PRINT,"\n Kill the process with pid %d\n", proc[i]->p_pid);
      proc_kill(proc[i], proc[i]->p_status);
   }
   return NULL;
}

void *procKillReverse(int arg1, void *arg2)
{

   int status = 0;
   proc_t *proc1 = NULL;
   kthread_t *thr1 = NULL;

   proc1 = proc_create("proc1");
   dbg(DBG_PRINT,"\n Process created is '%s' with pid %d \n",proc1->p_comm, proc1->p_pid);
   thr1 = kthread_create(proc1,createNkillReverse, 0, NULL);
   
   sched_make_runnable(thr1);

   while(do_waitpid(-1,0,&status) > 0);
   
   return NULL;  

}


/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
	      pci_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
        /* This little loop gives gdb a place to synch up with weenix.  In the
         * past the weenix command started qemu was started with -S which
         * allowed gdb to connect and start before the boot loader ran, but
         * since then a bug has appeared where breakpoints fail if gdb connects
         * before the boot loader runs.  See
         *
         * https://bugs.launchpad.net/qemu/+bug/526653
         *
         * This loop (along with an additional command in init.gdb setting
         * gdb_wait to 0) sticks weenix at a known place so gdb can join a
         * running weenix, set gdb_wait to zero  and catch the breakpoint in
         * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
         *
         * DANGER: if GDBWAIT != 0, and gdb is not running, this loop will never
         * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
         * you expect.
         */
        while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2)
{
        /* necessary to finalize page table information */
        pt_template_init();
	/*------------------------------------------------------------------------------------------------ */
	/*Creating a process idle process*/
	curproc = proc_create("idle_process");
	KASSERT(NULL != curproc); /* make sure that the "idle" process has been created 	successfully */
	dbg(DBG_PRINT,"\n(GRADING1A 1.a) idle process has been created successfully\n");
	KASSERT(PID_IDLE == curproc->p_pid); /* make sure that what has been created is the "idle" process */
	dbg(DBG_PRINT,"\n(GRADING1A 1.a) what has been created is the idle process\n");

	/*Creating a thread to run the idleproc_run routine*/
	curthr = kthread_create(curproc, idleproc_run, 0, NULL);
	KASSERT(NULL != curthr); /* make sure that the thread for the "idle" process has been created successfully */
	dbg(DBG_PRINT,"\n(GRADING1A 1.a) the thread for the idle process has been created successfully\n");
	/*joining the thread*/
	/* kthread_join(curthr, &(curthr->retval));*/

	/*New process's context call*/
	context_make_active(&(curthr->kt_ctx));



	/*------------------------------------------------------------------------------------------------*/
        /* NOT_YET_IMPLEMENTED("PROCS: bootstrap"); 

        panic("weenix returned to bootstrap()!!! BAD!!!\n");*/
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        NOT_YET_IMPLEMENTED("VFS: idleproc_run");
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif


#ifdef __SHADOWD__
        /* wait for shadowd to shutdown */
        shadowd_shutdown();
#endif

#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void)
{
        /*------------------------------------------------------------------------------------------------ */
	/*Creating a process init process*/
	proc_t *pr = proc_create("init_process");
	KASSERT(NULL !=pr );
        dbg(DBG_PRINT,"\n(GRADING1A 1.b) init process created\n");

	KASSERT(PID_INIT == pr->p_pid);
	dbg(DBG_PRINT,"\n(GRADING1A 1.b) init process pid correct\n");

	/*Creating a thread to run the initproc_run routine*/
	kthread_t *th = kthread_create(pr, initproc_run, 0, NULL);
	KASSERT( th != NULL);
        dbg(DBG_PRINT,"\n(GRADING1A 1.b) init process thread created\n");

	/*joining the thread*/
	/*kthread_join(th, &(th->retval));*/



	/*NOT_YET_IMPLEMENTED("PROCS: initproc_create");*/
	return th;
	/*---------------------------------------------------------------------------------------------------*/
}

#ifdef __DRIVERS__

        int faberTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1C): faber_test() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
		testproc(0, NULL);
            	return 0;
        }

        int sunghanTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1D 1): sunghan_test() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
		sunghan_test(0, NULL);
            	return 0;
        }

        int sunghanDeadlockTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1D 2): sunghan_deadlock_test() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
		sunghan_deadlock_test(0, NULL);
            	return 0;
        }


	int invalidPidTest(kshell_t *kshell, int argc, char **argv)
	{
		KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1E 1): invalidPidTest(): doWaitpid is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
		edgeDoWaitpidTest(-2, NULL);
            	return 0;
	}

	int idlePidTest(kshell_t *kshell, int argc, char **argv)
	{
		KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1E 2): idlePidTest(): doWaitpid is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
		edgeDoWaitpidTest(0, NULL);
            	return 0;
	}

        int muDeadlockTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1E 5): mutexDeadlockTest() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
                mutexDeadlockTest(0,NULL);
		
            	return 0;
        }
        
        
        int muLockTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1E 3): tryMutexLockTwice() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
                tryMutexLockTwice(0,NULL);
	
            	return 0;
        }

        int muUnlockTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1E 4): threadUnlocksMutex() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
                threadUnlocksMutex(0,NULL);
		
            	return 0;
        }
        
        int revProcKillTest(kshell_t *kshell, int argc, char **argv)
        {
        	KASSERT(kshell != NULL);
        	dbg(DBG_INIT, "(GRADING1E 6): procKillReverse() is invoked, argc = %d, argv = 0x%08x\n",
        	argc, (unsigned int)argv);
                procKillReverse(0,NULL);
		
            	return 0;
        }

        

#endif /* __DRIVERS__ */

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/bin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2)
{
        /*-----------------------------------------------------------------------*/
	
	#ifdef __DRIVERS__

        	kshell_add_command("faber", faberTest, "Runs faber test");
		kshell_add_command("sunghan", sunghanTest, "Runs sunghan test");
		kshell_add_command("sunghan_deadlock", sunghanDeadlockTest, "Runs sunghan deadlock test");
		kshell_add_command("invalid_pid", invalidPidTest, "Checks invalid pid, Should fail KASSERT");
		kshell_add_command("idle_pid", idlePidTest, "Runs edge condition of waitpid for IDLE proc");
                kshell_add_command("mutex_lock", muLockTest, "Try to lock same mutex twice, it should fail locking");
                kshell_add_command("mutex_unlock", muUnlockTest, "Process fails while trying to unlock a mutex which is locked by another thread");
		kshell_add_command("mutex_deadlock", muDeadlockTest, "Processes get stuck in deadlock trying to acquire mutex held by each other");
                kshell_add_command("reverse_kill", revProcKillTest, "Kill all the processes in reverse order");

        	kshell_t *kshell = kshell_create(0);
        	if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        	while (kshell_execute_next(kshell));
        	kshell_destroy(kshell);

	#endif /* __DRIVERS__ */
	/*testproc(0, NULL);*/
/*adding*/

        return 0;

	/*------------------------------------------------------------------------*/
	/*NOT_YET_IMPLEMENTED("PROCS: initproc_run");*/


	return NULL;
}
