#include "globals.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/kthread.h"
#include "proc/kmutex.h"

/*
 * IMPORTANT: Mutexes can _NEVER_ be locked or unlocked from an
 * interrupt context. Mutexes are _ONLY_ lock or unlocked from a
 * thread context.
 */

void
kmutex_init(kmutex_t *mtx)
{
        /* NOT_YET_IMPLEMENTED("PROCS: kmutex_init"); */
	mtx->km_holder = NULL;
	list_init(&(mtx->km_waitq.tq_list));
	mtx->km_waitq.tq_size = 0;
}

/*
 * This should block the current thread (by sleeping on the mutex's
 * wait queue) if the mutex is already taken.
 *
 * No thread should ever try to lock a mutex it already has locked.
 */
void
kmutex_lock(kmutex_t *mtx)
{
        /* NOT_YET_IMPLEMENTED("PROCS: kmutex_lock"); */
	KASSERT(curthr && (curthr != mtx->km_holder));
	dbg(DBG_PRINT,"\n(GRADING1A 5.a curthr %s: %d does not hold mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);
	if(mtx->km_holder == NULL) {
		dbg(DBG_PRINT,"\nkmutex_lock(): Thread '%s: %d' acquired the mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);
		mtx->km_holder = curthr;
	} else {
		dbg(DBG_PRINT,"\nkmutex_lock(): Thread '%s' blocked on the mutex\n", curthr->kt_proc->p_comm);
		sched_sleep_on(&(mtx->km_waitq));
	}
}

/*
 * This should do the same as kmutex_lock, but use a cancellable sleep
 * instead.
 */
int
kmutex_lock_cancellable(kmutex_t *mtx)
{
        /* NOT_YET_IMPLEMENTED("PROCS: kmutex_lock_cancellable"); */
	KASSERT(curthr && (curthr != mtx->km_holder));
	dbg(DBG_PRINT,"\n(GRADING1A 5.b curthr %s: %d does not hold mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);
	if(mtx->km_holder == NULL) {
		dbg(DBG_PRINT,"\nkmutex_lock_cancellable(): Thread '%s: %d' acquired the mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);
		mtx->km_holder = curthr;
	} else {
		dbg(DBG_PRINT,"\nkmutex_lock_cancellable(): Thread '%s: %d' blocked on the mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);
		return sched_cancellable_sleep_on(&(mtx->km_waitq));
	}
        return 0;
}

/*
 * If there are any threads waiting to take a lock on the mutex, one
 * should be woken up and given the lock.
 *
 * Note: This should _NOT_ be a blocking operation!
 *
 * Note: Don't forget to add the new owner of the mutex back to the
 * run queue.
 *
 * Note: Make sure that the thread on the head of the mutex's wait
 * queue becomes the new owner of the mutex.
 *
 * @param mtx the mutex to unlock
 */
void
kmutex_unlock(kmutex_t *mtx)
{
        /* NOT_YET_IMPLEMENTED("PROCS: kmutex_unlock"); */
	/* wakeupon  */
	KASSERT(curthr && (curthr == mtx->km_holder));
	dbg(DBG_PRINT,"\n(GRADING1A 5.c curthr %s: %d holds the mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);

	mtx->km_holder = sched_wakeup_on(&(mtx->km_waitq));
	dbg(DBG_PRINT,"\nkmutex_unlock(): Thread '%s: %d' released the mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);

	if(mtx->km_holder != NULL)
		dbg(DBG_PRINT,"\nkmutex_unlock(): Thread '%s: %d' acquired the mutex\n", (mtx->km_holder)->kt_proc->p_comm, (mtx->km_holder)->kt_proc->p_pid);

	KASSERT(curthr != mtx->km_holder);
	dbg(DBG_PRINT,"\n(GRADING1A.5.c curthr %s: %d does not hold the mutex\n", curthr->kt_proc->p_comm, curthr->kt_proc->p_pid);

}
