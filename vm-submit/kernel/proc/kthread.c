#include "config.h"
#include "globals.h"

#include "errno.h"

#include "util/init.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"

#include "mm/slab.h"
#include "mm/page.h"

kthread_t *curthr; /* global */
static slab_allocator_t *kthread_allocator = NULL;

#ifdef __MTP__
/* Stuff for the reaper daemon, which cleans up dead detached threads */
static proc_t *reapd = NULL;
static kthread_t *reapd_thr = NULL;
static ktqueue_t reapd_waitq;
static list_t kthread_reapd_deadlist; /* Threads to be cleaned */

static void *kthread_reapd_run(int arg1, void *arg2);
#endif

void
kthread_init()
{
	kthread_allocator = slab_allocator_create("kthread", sizeof(kthread_t));
	KASSERT(NULL != kthread_allocator);
}

/**
 * Allocates a new kernel stack.
 *
 * @return a newly allocated stack, or NULL if there is not enough
 * memory available
 */
static char *
alloc_stack(void)
{
	/* extra page for "magic" data */
	char *kstack;
	int npages = 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT);
	kstack = (char *)page_alloc_n(npages);

	return kstack;
}

/**
 * Frees a stack allocated with alloc_stack.
 *
 * @param stack the stack to free
 */
static void
free_stack(char *stack)
{
	page_free_n(stack, 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT));
}

/*
 * Allocate a new stack with the alloc_stack function. The size of the
 * stack is DEFAULT_STACK_SIZE.
 *
 * Don't forget to initialize the thread context with the
 * context_setup function. The context should have the same pagetable
 * pointer as the process.
 */


kthread_t *
kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2)
{
  
        KASSERT(NULL != p); 
        dbg(DBG_THR, "(GRADING1 3.a):Process to attach thread is not NULL.\n");
	
	
	

	kthread_t *thr =(kthread_t*) slab_obj_alloc(kthread_allocator);
	
	thr->kt_kstack = alloc_stack();
	thr->kt_retval = NULL;
	thr->kt_errno = NULL;
	thr->kt_proc = p;
	thr->kt_cancelled = 0;
	thr->kt_wchan = NULL;
	
	thr->kt_state = KT_NO_STATE;
	context_setup(&thr->kt_ctx, func, arg1, arg2,
	        thr->kt_kstack, DEFAULT_STACK_SIZE, p->p_pagedir);
	list_link_init(&(thr->kt_qlink));
	list_link_init(&(thr->kt_plink));

#ifdef __MTP__
	thr->kt_detached = 0;
	sched_queue_init(thr->kt_joinq);
#endif

	list_insert_tail(&p->p_threads, &thr->kt_plink);
	return thr;

}

void
kthread_destroy(kthread_t *t)
{
	KASSERT(t && t->kt_kstack);
	free_stack(t->kt_kstack);
	if (list_link_is_linked(&(t->kt_plink)))
		list_remove(&t->kt_plink);

	slab_obj_free(kthread_allocator, t);
}

/*
 * If the thread to be cancelled is the current thread, this is
 * equivalent to calling kthread_exit. Otherwise, the thread is
 * sleeping and we need to set the cancelled and retval fields of the
 * thread.
 *
 * If the thread's sleep is cancellable, cancelling the thread should
 * wake it up from sleep.
 *
 * If the thread's sleep is not cancellable, we do nothing else here.
 */
void
kthread_cancel(kthread_t *kthr, void *retval)
{
        KASSERT(NULL != kthr); /* should have thread */
        dbg(DBG_THR, "(GRADING1 3.b):Thread is not NULL.\n");
        
	if (curthr == kthr) {
		dbg(DBG_THR, "Canceling current thread.\n");
		kthread_exit(retval);
	}
	else{
		dbg(DBG_THR, "Canceling other thread.\n");
		kthr->kt_retval = retval;
		sched_cancel(kthr);
	}
}

/*
 * You need to set the thread's retval field, set its state to
 * KT_EXITED, and alert the current process that a thread is exiting
 * via proc_thread_exited.
 *
 * It may seem unneccessary to push the work of cleaning up the thread
 * over to the process. However, if you implement MTP, a thread
 * exiting does not necessarily mean that the process needs to be
 * cleaned up.
 */
void
kthread_exit(void *retval)
{
        KASSERT(!curthr->kt_wchan); /* queue should be empty */
        dbg(DBG_THR, "(GRADING1 3.c):Thread queue is Empty.\n");
  
        KASSERT(!curthr->kt_qlink.l_next && !curthr->kt_qlink.l_prev); /* queue should be empty */
        dbg(DBG_THR, "(GRADING1 3.c):Thread queue pointers are NULL.\n");
        
        
        KASSERT(curthr->kt_proc == curproc);
        dbg(DBG_THR, "(GRADING1 3.c):Thread's process is curproc.\n");
        
	curthr->kt_retval = retval;
	curthr->kt_state= KT_EXITED;
	proc_thread_exited(retval);
}

/*
 * The new thread will need its own context and stack. Think carefully
 * about which fields should be copied and which fields should be
 * freshly initialized.
 *
 * You do not need to worry about this until VM.
 */
kthread_t *
kthread_clone(kthread_t *thr)
{
	kthread_t* cthread = slab_obj_alloc(kthread_allocator);

	memcpy(&cthread->kt_ctx, &thr->kt_ctx, sizeof(context_t));
	cthread->kt_kstack= alloc_stack();

	cthread->kt_ctx.c_kstack = (uintptr_t)cthread->kt_kstack;
	cthread->kt_ctx.c_kstacksz = thr->kt_ctx.c_kstacksz;

	memcpy( (void*)cthread->kt_ctx.c_kstack, (const void*)thr->kt_ctx.c_kstack, thr->kt_ctx.c_kstacksz);
	cthread->kt_wchan = thr->kt_wchan;

	cthread->kt_retval = thr->kt_retval;

	cthread->kt_cancelled = thr->kt_cancelled;
	cthread->kt_errno = thr->kt_errno;

	cthread->kt_state = thr->kt_state;

	list_link_init(&cthread->kt_qlink);
	list_link_init(&cthread->kt_plink);

	if(cthread->kt_wchan){
		list_insert_head( &cthread->kt_wchan->tq_list, &cthread->kt_qlink);
		cthread->kt_wchan->tq_size++;
	}

	KASSERT(KT_RUN == thr->kt_state);
	dbg(DBG_ALL, "GRADING3 7.a: state of the thread is RUN\n");

	return cthread;
}

/*
 * The following functions will be useful if you choose to implement
 * multiple kernel threads per process. This is strongly discouraged
 * unless your weenix is perfect.
 */
#ifdef __MTP__
int
kthread_detach(kthread_t *kthr)
{
	NOT_YET_IMPLEMENTED("MTP: kthread_detach");
	return 0;
}

int
kthread_join(kthread_t *kthr, void **retval)
{
	NOT_YET_IMPLEMENTED("MTP: kthread_join");
	return 0;
}

/* ------------------------------------------------------------------ */
/* -------------------------- REAPER DAEMON ------------------------- */
/* ------------------------------------------------------------------ */
static __attribute__((unused)) void
		kthread_reapd_init()
{
	NOT_YET_IMPLEMENTED("MTP: kthread_reapd_init");
}
init_func(kthread_reapd_init);
init_depends(sched_init);

void
kthread_reapd_shutdown()
{
	NOT_YET_IMPLEMENTED("MTP: kthread_reapd_shutdown");
}

static void *
kthread_reapd_run(int arg1, void *arg2)
{
	NOT_YET_IMPLEMENTED("MTP: kthread_reapd_run");
	return (void *) 0;
}
#endif
