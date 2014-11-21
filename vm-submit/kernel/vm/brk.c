#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should NOT fail as the man page says. Instead,
 * "return" the current break. We use this to implement sbrk(0) without writing
 * a separate syscall. Look in user/libc/syscall.c if you're curious.
 *
 * Also, despite the statement on the manpage, you MUST support combined use
 * of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
	
	if(NULL == addr)
	{
		*ret=curproc->p_brk;
		return 0;
	}
	if((uint32_t)curproc->p_start_brk > (uint32_t)addr)
	{
		return -ENOMEM;
	}
	if(USER_MEM_HIGH < (uint32_t)addr )
	{
		return -ENOMEM;
	}

	uint32_t finalbrkPg = ADDR_TO_PN(addr);
	int retVal = 0;
	vmarea_t* vmarea_temp = NULL;
	uint32_t uintInitPg = 0;
	uint32_t initialbrkPg = ADDR_TO_PN(curproc->p_start_brk);

	

	if(0 == (vmmap_lookup(curproc->p_vmmap,initialbrkPg)))
	{
		/*vmarea_t *vmarea_temp;*/
		retVal = 0;
		retVal = do_mmap(PN_TO_ADDR(initialbrkPg),(size_t)PN_TO_ADDR(finalbrkPg-initialbrkPg),PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANON ,-1,0,(void**)&vmarea_temp);
		if(0 > retVal)
		{
			return retVal;
		}
		else
		    {
dbg_print("Going to end\n");
		goto lblEnd;
		    }
	}

	uintInitPg = ADDR_TO_PN(curproc->p_brk);

	if(uintInitPg == finalbrkPg )
		{
	        goto lblEnd;
		}

	if(uintInitPg < finalbrkPg )
	{

	        vmarea_temp = NULL;
	        vmarea_temp = vmmap_lookup(curproc->p_vmmap, uintInitPg);
		if(1 != vmmap_is_range_empty(curproc->p_vmmap,vmarea_temp->vma_end, (1 +  finalbrkPg-vmarea_temp->vma_end)))
		{
		        return -ENOMEM;
		}
		else{
		        vmarea_temp->vma_end = 1 +  finalbrkPg;
		                              goto lblEnd;
		}
			
	}
	else
	{
		vmmap_remove(curproc->p_vmmap,finalbrkPg+1,uintInitPg-finalbrkPg);
		goto lblEnd;
	}

	lblEnd:
	curproc->p_brk=addr;
	*ret = addr;
	return 0;

}


