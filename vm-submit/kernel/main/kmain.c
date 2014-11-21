#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"
#include "fs/file.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"
#include "mm/slab.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/cpuid.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/tty/virtterm.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/kshell/io.h"


GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);
static void       hard_shutdown(void);
static void printMessage(kshell_t *k, char* message);
static context_t bootstrap_context;

slab_allocator_t* MutexAllocator ;


/*  VM tests */
static int my_vfstest(kshell_t *k, int argc, char **argv);


kthread_t *thr1, *thr2, *thr3, *thr4;
kmutex_t *broadMtx;
proc_t *proc1, *proc2, *proc3, *proc4;
ktqueue_t * runqPtr;


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
	context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
	context_make_active(&bootstrap_context);

	panic("\nReturned to kmain()!!!\n");
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



	/*TeamCode - Start*/
	proc_t *idleProc = proc_create("idle");
	kthread_t * idleProcThread = kthread_create(idleProc, idleproc_run, 0,NULL);
	idleProcThread->kt_state = KT_RUN;
	curproc = idleProc;
	curthr = idleProcThread;

	KASSERT(NULL != curproc); /* make sure that the "idle" process has been created successfully */
	dbg(DBG_PROC, "(GRADING1 1.a):curproc is not NULL.\n");

	KASSERT(PID_IDLE == curproc->p_pid); /* make sure that what has been created is the "idle" process */
	dbg(DBG_PROC, "(GRADING1 1.a):Idle process has PID = 1.\n");

	KASSERT(NULL != curthr); /* make sure that the thread for the "idle" process has been created successfully */
	dbg(DBG_THR, "(GRADING1 1.a):curthr is not NULL.\n");

	context_make_active(&idleProcThread->kt_ctx);
	/*TeamCode - End*/

	panic("weenix returned to bootstrap()!!! BAD!!!\n");
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
	dbg(DBG_PRINT," IdleProc_run : Current Process : %s \n", curproc -> p_comm);
	int status;
	pid_t child;



	/* create init proc */
	kthread_t *initthr = initproc_create();

	init_call_all();
	GDB_CALL_HOOK(initialized);

	/* Create other kernel threads (in order) */

	if(vfs_root_vn == NULL)
	    {
	        dbg(DBG_PRINT, "VFS Root is NULL");
	    }
	
#ifdef __VFS__
	/* Once you have VFS remember to set the current working directory
	 * of the idle and init processes */

        int dev;
        int nullDev;
        int zeroDev;
        int ttyDev;

        curproc->p_cwd = vfs_root_vn;
        vref(curproc->p_cwd);

        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(initthr->kt_proc->p_cwd);

        if((dev = do_mkdir("/dev") < 0)) dbg(DBG_PRINT,"Did not create dev with error %d\n",dev);
        if((nullDev = do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID))< 0) dbg(DBG_PRINT,"Did not create null with error %d\n", nullDev);
        if((zeroDev = do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID))< 0) dbg(DBG_PRINT,"Did not create zero with error %d\n", zeroDev);
        if((ttyDev = do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2, 0)))< 0) dbg(DBG_PRINT,"Did not create tty0 with error %d\n", ttyDev);
	
	
	/* Here you need to make the null, zero, and tty devices using mknod */
	/* You can't do this until you have VFS, check the include/drivers/dev.h
	 * file for macros with the device ID's you will need to pass to mknod */
#endif

	/* Finally, enable interrupts (we want to make sure interrupts
	 * are enabled AFTER all drivers are initialized) */
	intr_enable();

	/* Run initproc */
	sched_make_runnable(initthr);
	/* Now wait for it */
	child = do_waitpid(-1, 0, &status);
	/*dbg(DBG_ALL," Received %d from dowait_pid\n", child);*/
	KASSERT(PID_INIT == child);

#ifdef __MTP__
	kthread_reapd_shutdown();
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
        proc_t *initProc = proc_create("init");
	KASSERT(NULL != initProc); /* pointer to the "init" process */
	dbg(DBG_PROC, "(GRADING1 1.b):initProc is not NULL.\n");

	KASSERT(initProc->p_pid == PID_INIT );
	dbg(DBG_PROC, "(GRADING1 1.b):init process does not have PID = PID_INIT.\n");

	kthread_t *initProc_Thread = kthread_create(initProc, initproc_run, 0, NULL);

	KASSERT(initProc_Thread != NULL);
	dbg(DBG_THR, "(GRADING1 1.b):initProc_Thread is not NULL. \n");

	return initProc_Thread;
	/*return NULL;*/
}

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


static int helloTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/usr/bin/hello", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}
static int shTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/bin/sh", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}
static int spinTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/usr/bin/spin", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}

/*from here*/
static int segTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/usr/bin/segfault", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}
static int lsTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/bin/ls", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}

static int haltTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/sbin/halt", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}

static int edTest (kshell_t *k, int argc1, char **argv1)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };

  kernel_execve("/bin/ed", argv, envp);      

  proc_t* p = NULL;
  list_iterate_begin(&curproc->p_children,p,proc_t,p_child_link){
    do_waitpid(p->p_pid,0,&p->p_status);
  }list_iterate_end();
  return 0;
}


void* vm_test(long int arg1, void* arg2)
{
  char *argv[] = { NULL };
  char *envp[] = { NULL };
  kshell_add_command("testhello", helloTest, "Launches the hello world userland program");
  kshell_add_command("testsh", shTest, "Launches the shell userland program");
  kshell_add_command("testspin", spinTest, "Launches the spin userland program(infinite loop)");
  kshell_add_command("testls", lsTest, "Launches the ls userland program");
  kshell_add_command("testhalt", haltTest, "Launches the halt userland program to halt system");
  kshell_add_command("testEd", edTest, "Launches the Editor userland program");
  
  kernel_execve("/sbin/init", argv, envp);
  return 0;

}



static void *
initproc_run(int arg1, void *arg2)
{
    
#ifdef __VM__

  proc_t* vm = proc_create("VM_Test");
  kthread_t* kt = kthread_create(vm,(kthread_func_t)vm_test,0,NULL);
  sched_make_runnable(kt);
  

#endif  /* __VM__ */

  list_t* temp = &curproc->p_children;
  while(1)
    {
      if(list_empty(&curproc->p_children))
        break;
      proc_t* p = NULL;   
      p = list_item(temp->l_next,proc_t,p_child_link);
      do_waitpid(p->p_pid,0,&p->p_status);
    }
  return NULL;
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

/*

static int
my_vfstest(kshell_t *k, int argc, char **argv){
        printMessage(k, "VFS Tests Started\n");
        int ret = vfstest_main(argc, argv);
        printMessage(k, "VFS Tests Finished\n");
        return ret;
}
*/

/*
static void printMessage(kshell_t *k, char* message)
{
    kprintf(k, "%s\n",message);
    dbg_print("%s\n",message);
}*/
