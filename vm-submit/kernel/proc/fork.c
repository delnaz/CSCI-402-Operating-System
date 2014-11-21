#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
  /* Pointer argument and dummy return address, and userland dummy return
   * address */
  uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
  *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
  memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
  return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
  
  KASSERT(regs != NULL);
  dbg(DBG_ALL, "GRADING3 5.a: regs is not NULL \n");

 
  KASSERT(curproc != NULL);
  dbg(DBG_ALL, "GRADING3 5.a: curproc is not NULL\n");

 
  KASSERT(curproc->p_state == PROC_RUNNING);
  dbg(DBG_ALL, "GRADING3 5.a: state of current process is running \n");

  proc_t* newproc = proc_create("newChild");

  KASSERT(newproc->p_state == PROC_RUNNING);
  dbg(DBG_ALL, "GRADING3 5.a: state of new child process is running\n");


  KASSERT(newproc->p_pagedir != NULL);
  dbg(DBG_ALL, "GRADING3 5.a: page directory of the new child process is not NULL \n");


  vmarea_t* vmarea_newChild = NULL;
  vmarea_t* vmareaP = NULL;
  mmobj_t* sh_p = NULL; /*referes to shadow of parent*/
  mmobj_t* sh_c = NULL; /*referes to shadow of child*/
  int fCounter=0;
  newproc->p_vmmap = vmmap_clone(curproc->p_vmmap);

  
  list_link_t* listObj = curproc->p_vmmap->vmm_list.l_next;
  list_iterate_begin(&newproc->p_vmmap->vmm_list, vmarea_newChild, vmarea_t, vma_plink)
    {
      vmareaP = list_item(listObj, vmarea_t,vma_plink);
      /*here vmareaP refres to parent's vmarea*/			  
      if(!(MAP_SHARED & vmareaP->vma_flags ))
	{
              sh_p = shadow_create();
                    sh_c = shadow_create();
                    sh_p->mmo_shadowed = vmareaP->vma_obj;
                    sh_c->mmo_shadowed = vmareaP->vma_obj;
                    
                    sh_c->mmo_un.mmo_bottom_obj = vmareaP->vma_obj->mmo_un.mmo_bottom_obj;
                    sh_p->mmo_un.mmo_bottom_obj = vmareaP->vma_obj->mmo_un.mmo_bottom_obj;

                            
                    
                    list_insert_tail(&sh_c->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &vmarea_newChild->vma_olink);
                                            
                    vmareaP->vma_obj->mmo_ops->ref(vmareaP->vma_obj);
                                            
                    
                    vmarea_newChild->vma_obj = sh_c;
                  
                    vmareaP->vma_obj = sh_p;
                    
                    pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vmareaP->vma_start), (uintptr_t)PN_TO_ADDR(vmareaP->vma_end));                                 
                                            
	}
      else
	{

              vmarea_newChild->vma_obj = vmareaP->vma_obj;
              vmarea_newChild->vma_obj->mmo_ops->ref(vmarea_newChild->vma_obj);
              list_insert_tail(&(vmareaP->vma_obj->mmo_un.mmo_vmas),&vmarea_newChild->vma_olink);

	}
      listObj= listObj->l_next;
			
    }list_iterate_end();
  
    kthread_t* threadIterator = NULL;
    kthread_t* newthr = NULL;
    tlb_flush_all();

  
  
  list_iterate_begin( &curproc->p_threads, threadIterator, kthread_t, kt_plink)
    {
       newthr = kthread_clone(threadIterator);

      
      KASSERT(newthr->kt_kstack != NULL);
      dbg(DBG_ALL, "GRADING3 5.a: kernel stack of the thread is not NULL \n");

      newthr->kt_proc = newproc;
      newthr->kt_ctx.c_eip = (uint32_t)userland_entry;
      regs->r_eax = 0;
      newthr->kt_ctx.c_pdptr = newproc->p_pagedir;
      list_insert_tail(&newproc->p_threads, &newthr->kt_plink);
           
      
      newthr->kt_ctx.c_esp = fork_setup_stack(regs, newthr->kt_kstack);
      if(curthr == threadIterator )
	{
	  sched_make_runnable(newthr);
	}
    }list_iterate_end();
		
  
    fCounter=0;
  for(;fCounter<NFILES;fCounter++)
    {
	  newproc->p_files[fCounter]=curproc->p_files[fCounter];
      if(NULL != curproc->p_files[fCounter])
	fref(curproc->p_files[fCounter]);
    }

  
  newproc->p_cwd=curproc->p_cwd;
  vget(newproc->p_cwd->vn_fs,newproc->p_cwd->vn_vno);

  
  
  newproc->p_start_brk=curproc->p_start_brk;
  newproc->p_brk=curproc->p_brk;
		
  return newproc->p_pid;
}

