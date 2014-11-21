#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"
 
#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
  list_init(&_proc_list);
  proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
  KASSERT(proc_allocator != NULL);
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
  proc_t *p;
  
  pid_t pid = next_pid;
  while (1) {
  failed:
    list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
	
      if (p->p_pid == pid) {
	if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid) {
	  return -1;
	} else {
	  goto failed;
	}
      }
    } list_iterate_end();
    next_pid = (pid + 1) % PROC_MAX_COUNT;
    return pid;
  }
}


/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
  proc_t *pObj = (proc_t *)slab_obj_alloc(proc_allocator);
  KASSERT(pObj != NULL && "proc_create : Process created is NULL");

  pObj->p_pid = _proc_getid();
  
  KASSERT(PID_IDLE != pObj->p_pid || list_empty(&_proc_list)); /* pid can only be PID_IDLE if this is the first process */
  dbg(DBG_PROC, "(GRADING1 2.a):pid can only be PID_IDLE if this is the first process.\n");
  
  KASSERT(PID_INIT != pObj->p_pid || PID_IDLE == curproc->p_pid); /* pid can only be PID_INIT when creating from idle process */
  dbg(DBG_PROC, "(GRADING1 2.a):pid can only be PID_INIT when creating from idle process.\n");
  
  strncpy(pObj->p_comm, name, PROC_NAME_LEN);
  list_init(&(pObj->p_threads));
  list_init(&(pObj->p_children));

  if(PID_IDLE == pObj->p_pid){
      pObj->p_pproc = pObj;
  }else {
      pObj->p_pproc = curproc;
  }


  pObj->p_status = 0;
  pObj->p_state = PROC_RUNNING;
  sched_queue_init(&(pObj->p_wait));
  if(PID_IDLE != pObj->p_pid){
      pObj->p_pagedir = pt_create_pagedir();
  }else{
      pObj->p_pagedir = pt_get();
  }


  list_link_init(&(pObj->p_list_link));
  list_link_init(&(pObj->p_child_link));

  /*VFS code*/
  int counter;
  for(counter = 0; counter<NFILES; counter++){
      pObj->p_files[counter] = NULL;
  }
  pObj->p_cwd = vfs_root_vn;

  if ((pObj->p_pid != 0 && pObj->p_pproc->p_pid != 0 ) )
  {
          pObj -> p_cwd = curproc->p_cwd;
          vref (pObj->p_cwd);
  }

  /*VM code*/
  pObj->p_brk = NULL;
  pObj->p_start_brk = NULL;
  pObj->p_vmmap = NULL;


  if(PID_INIT == pObj->p_pid){
      proc_initproc = pObj;
  }

  list_insert_tail(&_proc_list, &(pObj->p_list_link));

  if(PID_IDLE != pObj->p_pid){
      list_insert_tail(&(pObj->p_pproc->p_children),&(pObj->p_child_link));
  }
  
#ifdef __VM__
  pObj->p_vmmap = vmmap_create();
  pObj->p_vmmap->vmm_proc = pObj;
#endif  

  return pObj;
}


/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
/*void
proc_cleanup(int status)
{
    int i = 0;

  KASSERT(proc_initproc);
  KASSERT(curproc->p_pid > 0);
  KASSERT(curproc->p_pproc);

  
  if(proc_initproc)
    vput(curproc->p_cwd);

  for(i=0;i<NFILES;i++)
      {
        if(curproc->p_files[i])
          fput(curproc->p_files[i]);
      }
  
  if (curproc->p_vmmap != NULL)
      vmmap_destroy(curproc->p_vmmap);
  proc_t* procTemp = NULL;
  list_iterate_begin(&curproc->p_children, procTemp, proc_t, p_child_link) {
    list_remove(&procTemp->p_child_link);
    procTemp->p_pproc = proc_initproc;
    list_insert_tail(&proc_initproc->p_children,&procTemp->p_child_link);   
  } list_iterate_end();

  curproc->p_state = PROC_DEAD;
}*/


void
proc_cleanup(int status)
{
  


  KASSERT(proc_initproc);

  
  KASSERT(curproc->p_pid > 0);
  KASSERT(curproc->p_pproc);

  int counter= 0;
  
  for(counter=0;counter<NFILES;counter++)
    {
      if(curproc->p_files[counter])
        fput(curproc->p_files[counter]);
    }
  if(proc_initproc)
    vput(curproc->p_cwd);



  vmmap_destroy(curproc->p_vmmap);



sched_wakeup_on(&curproc->p_pproc->p_wait);
    
  
proc_t* iterator = NULL;
  list_iterate_begin(&curproc->p_children, iterator, proc_t, p_child_link) {
    list_remove(&iterator->p_child_link);
    iterator->p_pproc = proc_initproc;
    list_insert_tail(&proc_initproc->p_children,&iterator->p_child_link);   
  } list_iterate_end();

  curproc->p_state = PROC_DEAD;

}



/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
  KASSERT(p);
  kthread_t* tempThread = NULL;
  if( curproc == p)
    do_exit(status);

  else
    {
      
      list_iterate_begin(&p->p_threads, tempThread, kthread_t, kt_plink) {
        kthread_cancel(tempThread, (void*)status);         
      } list_iterate_end();
    }
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
  proc_t* iterator = NULL;

  list_iterate_begin(&_proc_list, iterator, proc_t, p_list_link)
    {
      if((curproc != iterator )  && (PID_IDLE != iterator->p_pproc->p_pid ) && ( PID_IDLE != iterator->p_pid ))
        {
          proc_kill(iterator, iterator->p_status);           
        }
    } list_iterate_end();
  list_iterate_begin(&curproc->p_children, iterator, proc_t, p_child_link)
    {
      do_waitpid(iterator->p_pid,0, &iterator->p_status);           
    } list_iterate_end();
  do_exit(0);

}


proc_t *
proc_lookup(int pid)
{
  proc_t *p;
  list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
    if (p->p_pid == pid) {
      return p;
    }
  } list_iterate_end();
  return NULL;
}

list_t *
proc_list()
{  
  return &_proc_list;
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
#ifdef __MTP__
  proc_cleanup( retval );
#endif
  proc_cleanup((int)retval);	
  sched_switch();

}

void disposeZombie(proc_t* pZombie) {


        kthread_t * t = NULL;
        t= list_head(&(pZombie->p_threads), kthread_t, kt_plink);
        KASSERT(KT_EXITED == t->kt_state);
        dbg(DBG_ALL, "(GRADING1 2.c.3):points thread to be destroyed\n");

        kthread_destroy(list_head(&(pZombie->p_threads), kthread_t, kt_plink));
        pZombie->p_pproc=NULL;
        KASSERT(NULL != pZombie->p_pagedir);
        dbg(DBG_ALL, "(GRADING1 2.c.4):pagedir not null\n");
        pt_destroy_pagedir(pZombie->p_pagedir);
        list_remove(&(pZombie->p_child_link));
        list_remove(&(pZombie->p_list_link));


        pZombie->p_start_brk = NULL;
        pZombie->p_cwd = NULL;
        pZombie->p_brk = NULL;
        pZombie->p_vmmap = NULL;

        slab_obj_free(proc_allocator, pZombie);

}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{

        KASSERT( options == 0 && "Options in do_waitpid cannot be zero");
        KASSERT((pid == -1 || pid > 0) && "PIDs other than -1 and positive numbers are not supported.");

        proc_t *iterator = NULL;

        if(list_empty(&(curproc->p_children)))
        {
                return -ECHILD;
        }
        pid_t retpid=0;
        int elsecond = 0;
        int childDied = 0;
        if(pid == -1)
        {
                while(1)
                {
                        childDied = 0;
                        list_iterate_begin(&(curproc->p_children), iterator, proc_t, p_child_link) {
                                if( PROC_DEAD== iterator->p_state)
                                {
                                        if (status != NULL)
                                            *status = iterator->p_status;
                                        childDied = 1;
                                        retpid = iterator->p_pid;
                                        disposeZombie(iterator);

                                        return retpid;
                                }
                        } list_iterate_end();
                        if(childDied != 1)
                        {
                                sched_sleep_on(&(curproc->p_wait));
                        }
                }
        }
        else if(pid > 0)
        {
            proc_t* child ;
            list_iterate_begin(&(curproc->p_children), iterator, proc_t, p_child_link)
            {
              if(iterator->p_pid == pid){
                  KASSERT(iterator!= NULL && "process is null");
                  dbg(DBG_ALL, "(GRADING1 2.c.1):process not null\n");
                  KASSERT(-1 == pid || iterator->p_pid == pid);
                  dbg(DBG_ALL, "(GRADING1 2.c.2): found the process\n");

                  child = iterator;
                  elsecond = 1;

              }
            } list_iterate_end();
            iterator = child;

                if(elsecond !=1)
                {
                        return -ECHILD;
                }

                while(1)
                {
                        if(PROC_DEAD == iterator->p_state )
                        {
                                retpid = iterator->p_pid;
                                 if (status != NULL)
                                     *status = iterator->p_status;
                                disposeZombie(iterator);
                                return retpid;
                        }
                        else
                        {
                                sched_sleep_on(&(curproc->p_wait));
                        }
                }
        }
        return 0;
}


/*
 * Cancel all threads, join with them, and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
  kthread_t *myThread;
  list_iterate_begin(&curproc->p_threads, myThread, kthread_t, kt_plink){
    if(myThread != curthr){
        kthread_cancel(myThread, &status);
    }
  }list_iterate_end();
  kthread_cancel(curthr, &status);
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
  const proc_t *p = (proc_t *) arg;
  size_t size = osize;
  proc_t *child;

  KASSERT(NULL != p);
  KASSERT(NULL != buf);

  iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
  iprintf(&buf, &size, "name:         %s\n", p->p_comm);
  if (NULL != p->p_pproc) {
    iprintf(&buf, &size, "parent:       %i (%s)\n",
	    p->p_pproc->p_pid, p->p_pproc->p_comm);
  } else {
    iprintf(&buf, &size, "parent:       -\n");
  }

#ifdef __MTP__
  int count = 0;
  kthread_t *kthr;
  list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
    ++count;
  } list_iterate_end();
  iprintf(&buf, &size, "thread count: %i\n", count);
#endif

  if (list_empty(&p->p_children)) {
    iprintf(&buf, &size, "children:     -\n");
  } else {
    iprintf(&buf, &size, "children:\n");
  }
  list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
    iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
  } list_iterate_end();

  iprintf(&buf, &size, "status:       %i\n", p->p_status);
  iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
  if (NULL != p->p_cwd) {
    char cwd[256];
    lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
    iprintf(&buf, &size, "cwd:          %-s\n", cwd);
  } else {
    iprintf(&buf, &size, "cwd:          -\n");
  }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
  iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
  iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

  return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
  size_t size = osize;
  proc_t *p;

  KASSERT(NULL == arg);
  KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
  iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
  iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

  list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
    char parent[64];
    if (NULL != p->p_pproc) {
      snprintf(parent, sizeof(parent),
	       "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
    } else {
      snprintf(parent, sizeof(parent), "  -");
    }

#if defined(__VFS__) && defined(__GETCWD__)
    if (NULL != p->p_cwd) {
      char cwd[256];
      lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
      iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
	      p->p_pid, p->p_comm, parent, cwd);
    } else {
      iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
	      p->p_pid, p->p_comm, parent);
    }
#else
    iprintf(&buf, &size, " %3i  %-13s %-s\n",
	    p->p_pid, p->p_comm, parent);
#endif
  } list_iterate_end();
  return size;
}
