#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/stat.h"


#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
		int fd, off_t off, void **ret)
{
	vmarea_t* vmArea;
	int32_t retVal;
	file_t* tempFile;
	vnode_t* tempNode;
	uint32_t mapPrivate = flags & MAP_PRIVATE;
	uint32_t mapShared = flags & MAP_SHARED;
	int result;


	if(!(PAGE_ALIGNED(off)))
		return -EINVAL;

	if(flags & ~(MAP_ANON|MAP_FIXED|MAP_PRIVATE|MAP_SHARED))
		return -ENOTSUP;


	if(!(PAGE_ALIGNED(len))){
		len = (uint32_t)PN_TO_ADDR(ADDR_TO_PN(len)+1);
	}

	if(0>=len)
		return -EINVAL;

	uint32_t endPoint = (uint32_t)addr + len;

	if(!((!mapShared && mapPrivate) || (mapShared && !mapPrivate)))
		return -EINVAL;

	if(!(MAP_FIXED & flags)){
		if( 0 > (retVal = vmmap_find_range(curproc->p_vmmap, ADDR_TO_PN(len), VMMAP_DIR_LOHI)))
			return -EINVAL;
		if(!ADDR_TO_PN(len))
			return -EINVAL;
		addr = PN_TO_ADDR(retVal);
	}
	else{
		if((endPoint > USER_MEM_HIGH) || ((uint32_t)addr > USER_MEM_HIGH) || ((uint32_t)addr < USER_MEM_LOW))
			return -EINVAL;
		if(!(PAGE_ALIGNED(addr)))
			return -EINVAL;
	}

	if((fd > MAX_FILES) || (fd < 0)){
		if(!(MAP_ANON & flags))
			return -EBADF;
	}
	else
	{
		if(MAP_ANON & flags)
			tempFile = NULL;
		else
			tempFile = fget(fd);

		if(!tempFile){
			if(!(MAP_ANON & flags))
				return -EBADF;
		}
		else{

			if(NFILES < ADDR_TO_PN(len)+ADDR_TO_PN(off))
				return -EOVERFLOW;

			if( (FMODE_APPEND == tempFile->f_mode) && (PROT_WRITE & prot) )
			      goto chkpt;
			if(mapPrivate && !(FMODE_READ & tempFile->f_mode))
			      goto chkpt;
			if(!( (S_ISCHR(tempFile->f_vnode->vn_mode))  ||  (S_ISREG(tempFile->f_vnode->vn_mode)) ))
			      goto chkpt;
			if(mapShared && (PROT_WRITE & prot) && (!(tempFile->f_mode & FMODE_WRITE) || !(tempFile->f_mode & FMODE_READ)))
			      goto chkpt;

			if(!tempFile->f_vnode->vn_ops->mmap){
				fput(tempFile);
				return -ENODEV;
			}
		}
	}


	if(!(MAP_ANON & flags))
		tempNode = tempFile->f_vnode;
	else
		tempNode = NULL;


	result = vmmap_map( curproc->p_vmmap, tempNode, ADDR_TO_PN(addr),
			ADDR_TO_PN(len),prot,flags, off, VMMAP_DIR_LOHI, &vmArea);

	*ret = PN_TO_ADDR(vmArea->vma_start);

	if(!(MAP_ANON & flags))
		fput(tempFile);

	tlb_flush_range((uintptr_t)addr,len);


	KASSERT(NULL != curproc->p_pagedir);
	dbg(DBG_ALL, "GRADING3 6.a: page directory of the current process is not NULL\n");

	return result;
	chkpt:
	  fput(tempFile);
	  return -EACCES;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
	if(!(PAGE_ALIGNED(addr)))
		return -EINVAL;

	if(!(PAGE_ALIGNED(len))){
		len = (uint32_t)PN_TO_ADDR(ADDR_TO_PN(len)+1);
	}

	uint32_t endPoint = (uint32_t)addr + len;
	if(0 >= len)
			return -EINVAL;

	if( (endPoint > USER_MEM_HIGH) || ((uint32_t)addr > USER_MEM_HIGH) || ((uint32_t)addr < USER_MEM_LOW) )
		return -EINVAL;


	vmmap_remove(curproc->p_vmmap, ADDR_TO_PN(addr), ADDR_TO_PN(len));
	tlb_flush_all();

	KASSERT(NULL != curproc->p_pagedir);
	dbg(DBG_ALL, "GRADING3 6.b: page directory of the current process\n");
	return 0;
}

