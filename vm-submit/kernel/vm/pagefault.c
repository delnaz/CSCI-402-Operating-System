#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"
#include "api/access.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{

  int accessRight = 0;
  pframe_t* tempPageframe = NULL;
  
  vmarea_t* area_lookup = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(vaddr));
  if(NULL == area_lookup)
    {
      
      curproc->p_status = EFAULT;
      kthread_exit(&curproc->p_status);
      return;
    }
  if(FAULT_WRITE & cause)
    accessRight = PROT_WRITE | accessRight;
  if(FAULT_EXEC & cause  )
    accessRight = PROT_EXEC | accessRight;
  
  if(FAULT_RESERVED & cause)
    accessRight = PROT_NONE | accessRight;
  if(FAULT_PRESENT & cause)
    accessRight = PROT_READ | accessRight;
		
  if(0 == addr_perm(curproc, (const void*)vaddr, accessRight))		
    {
     
      curproc->p_status = EFAULT;
      kthread_exit(&curproc->p_status);
      return;
    }

  uint32_t pageTableFlags = 0;
  
  if(0 == area_lookup->vma_obj->mmo_shadowed)
    {
      
      pframe_get( area_lookup->vma_obj,  area_lookup->vma_off + ADDR_TO_PN(vaddr) - area_lookup->vma_start, &tempPageframe);
    }
  
  else
    {
  
  
      area_lookup->vma_obj->mmo_ops->lookuppage(area_lookup->vma_obj,  area_lookup->vma_off + ADDR_TO_PN(vaddr) - area_lookup->vma_start , FAULT_WRITE & cause, &tempPageframe);
  
    }
   pageTableFlags = PT_PRESENT | PT_USER;
  if(FAULT_WRITE & cause)
      pageTableFlags = PT_WRITE | pageTableFlags;
  		  
  pt_map(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(ADDR_TO_PN(vaddr)), pt_virt_to_phys((uint32_t)tempPageframe->pf_addr), PD_WRITE | PD_PRESENT | PD_USER, pageTableFlags);
  }
