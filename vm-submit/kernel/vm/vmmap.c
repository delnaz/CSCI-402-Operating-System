#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "limits.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
  vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
  KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
  vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
  KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{       /* didn't initialize links of list*/
  vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
  if (newvma) {
    newvma->vma_vmmap = NULL;
  }
  return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
  KASSERT(NULL != vma);
  slab_obj_free(vmarea_allocator, vma);
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{

  vmmap_t* newObj = (vmmap_t *)slab_obj_alloc(vmmap_allocator); /*pankaj*/
  KASSERT(newObj && "could not allocate memory");
  newObj->vmm_proc = NULL;
  list_init(&(newObj->vmm_list));
  return newObj;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{


  KASSERT(NULL != map);
  dbg(DBG_ALL, "GRADING3 1.a: vmmap_t map is not NULL \n");

  vmarea_t* iterator =NULL;
  list_iterate_begin( &map->vmm_list, iterator, vmarea_t, vma_plink)
    {
      vmmap_remove(map,iterator->vma_start,iterator->vma_end - iterator->vma_start);
			  
    }list_iterate_end();
		
  slab_obj_free(vmmap_allocator, map);
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{

 vmarea_t* iterator = NULL;

  
  KASSERT(NULL != map && NULL != newvma);
  dbg(DBG_ALL, "GRADING3 1.b: vmmap_t map and  vmarea_t newvma is not NULL \n");


  KASSERT(NULL == newvma->vma_vmmap);
  dbg(DBG_ALL, "GRADING3 1.b: vmarea_t newvma->vma_vmmap is NULL \n");


  KASSERT(newvma->vma_start < newvma->vma_end);/*which means end shouldn't be inclusive*/
  dbg(DBG_ALL, "GRADING3 1.b: vmarea_t newvma->vma_vmastart is less than newvma->vma_end \n");


  KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
  dbg(DBG_ALL, "GRADING3 1.b: ADDR_TO_PN(USER_MEM_LOW) is less than or equal to newvma->vma_vmastart and ADDR_TO_PN(USER_MEM_HIGH) is greater than or equal to newvma->vma_end \n");
  


  if(!vmmap_is_range_empty(map, newvma->vma_start, newvma->vma_end - newvma->vma_start))
    return ;
  
   list_iterate_begin(&map->vmm_list, iterator, vmarea_t,vma_plink )
    {
	  if(!( iterator->vma_start < newvma->vma_end))
      	{
	  list_insert_before(&iterator->vma_plink, &newvma->vma_plink);
	  newvma->vma_vmmap = map;
	  return;
      	}
    }list_iterate_end();

  list_insert_tail(&map->vmm_list,&newvma->vma_plink);
  newvma->vma_vmmap = map;        
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{


	KASSERT(NULL != map);
	dbg(DBG_ALL, "GRADING3 1.c: vmmap_t map is not NULL \n");


	KASSERT(0 < npages);
	dbg(DBG_ALL, "GRADING3 1.c: uint32_t npages are greater than zero\n");
	vmarea_t* iterator=NULL;
	uint32_t memLowAddr;
	uint32_t memHighAddr ;
	vmarea_t* vmaareaTemp=NULL;
	if(VMMAP_DIR_LOHI != dir)
	{
		vmaareaTemp = NULL;
		memHighAddr = ADDR_TO_PN(USER_MEM_HIGH);
		list_iterate_reverse(&map->vmm_list, vmaareaTemp, vmarea_t, vma_plink)
		{
			if(npages > (memHighAddr - vmaareaTemp->vma_end)  )
			{
				memHighAddr = vmaareaTemp->vma_start;
			}
			else
			{

				return memHighAddr - npages;
			}

		}list_iterate_end();
		if(   ADDR_TO_PN(USER_MEM_LOW) > (memHighAddr- npages ) )
			return -1;
		else

			return memHighAddr - npages;

	}
	else
	{




		memLowAddr = ADDR_TO_PN(USER_MEM_LOW);

		list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink)
		{
			if( npages > (iterator->vma_start - memLowAddr))
			{
				memLowAddr = iterator->vma_end;
			}
			else
			{

				return memLowAddr;

			}

		}list_iterate_end();

		if(ADDR_TO_PN(USER_MEM_HIGH) < (npages + memLowAddr ))
		{
			return -1;
		}
		else

		{
			return memLowAddr;
		}
	}

}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
  
  
  KASSERT(NULL != map);
  dbg(DBG_ALL, "GRADING3 1.d: vmmap_t map is not NULL \n");

  vmarea_t* iterator = NULL;
  list_iterate_begin(&map->vmm_list, iterator, vmarea_t,vma_plink )
    {
      if(( iterator->vma_start <= vfn)  &&  ( iterator->vma_end  > vfn ) )
	return iterator;
    }list_iterate_end();
  
  return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{

  vmmap_t* newMapObj = vmmap_create();

  if(NULL == newMapObj)
    return NULL;
  vmarea_t* iterator=NULL;
  KASSERT(map);		

  list_iterate_begin( &map->vmm_list, iterator, vmarea_t, vma_plink)
    {
      vmarea_t* vma = vmarea_alloc();
      vma->vma_end = iterator->vma_end;
      vma->vma_prot = iterator->vma_prot;
      vma->vma_start = iterator->vma_start;
      vma->vma_obj = NULL;


      vma->vma_off = iterator->vma_off;

      vma->vma_flags = iterator->vma_flags;
      list_link_init(&vma->vma_olink);
      list_link_init(&vma->vma_plink);

      list_insert_tail(&newMapObj->vmm_list, &vma->vma_plink);
      vma->vma_vmmap = newMapObj;
    }list_iterate_end();
				
  return newMapObj;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
	vmarea_t* vmareaObj = NULL;
	KASSERT(NULL != map);
  dbg(DBG_ALL, "GRADING3 1.f: vmmap_t map is not NULL \n");

 
  KASSERT(0 < npages);
  dbg(DBG_ALL, "GRADING3 1.f: npages is greater than zero \n");

 
  KASSERT(!(~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot));
  dbg(DBG_ALL, "GRADING3 1.f: Valid protection flag provided \n");

 
  KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
  dbg(DBG_ALL, "GRADING3 1.f: if flags are map shared or private \n");

 
  KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
  dbg(DBG_ALL, "GRADING3 1.f: lopage is zero or greater than or equal to USER_MEM_LOW \n");

 
  KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
  dbg(DBG_ALL, "GRADING3 1.f: lopage is equal to zero or total no of lopages and npages are less than or equal to USER_MEM_HIGH \n");

 
  KASSERT(PAGE_ALIGNED(off));
  dbg(DBG_ALL, "GRADING3 1.f: PAGE ALIGNED is off \n");
	

  int retVal = 0;
  vmareaObj = vmarea_alloc();
  if(0 != lopage )
    {
	  if(!vmmap_is_range_empty(map, lopage, npages))
	 	{

	 	  vmmap_remove( map, lopage, npages);
	 	  KASSERT(vmmap_is_range_empty(map,lopage,npages));
	 	}
	       vmareaObj->vma_start = lopage;

    }

  else
    {

	  retVal = vmmap_find_range(map, npages, dir);
      if(0 < retVal)
    	  vmareaObj->vma_start = retVal;
      else

      return -ENOMEM;


    }
		
  vmareaObj->vma_end = npages + vmareaObj->vma_start ;
  vmareaObj->vma_flags = flags;
  vmareaObj->vma_prot = prot;
  vmareaObj->vma_off = off;
  list_link_init(&vmareaObj->vma_olink);
  list_link_init(&vmareaObj->vma_plink);


  if(!(MAP_PRIVATE & flags ))
  {

	  if(NULL != file)
	  {
		  file->vn_ops->mmap(file, vmareaObj, &(vmareaObj->vma_obj));
		  list_insert_tail(&(vmareaObj->vma_obj->mmo_un.mmo_vmas),
				  &(vmareaObj->vma_olink));
	  }


  }

  else
  {



	  vmareaObj->vma_obj = shadow_create();


	  	  if(!file)
	  	  {
	  		  vmareaObj->vma_obj->mmo_un.mmo_bottom_obj = anon_create();
	  		  vmareaObj->vma_obj->mmo_shadowed = vmareaObj->vma_obj->mmo_un.mmo_bottom_obj;
	  	  }

	  	  else
	  	  {


	  		  file->vn_ops->mmap(file, vmareaObj, &(vmareaObj->vma_obj->mmo_shadowed));
	  		  vmareaObj->vma_obj->mmo_un.mmo_bottom_obj = vmareaObj->vma_obj->mmo_shadowed;


	  	  }
	  	  list_insert_tail(&(vmareaObj->vma_obj->mmo_shadowed->mmo_un.mmo_vmas),
	  			  &(vmareaObj->vma_olink));


  }
		

  if(NULL != new)
    *new = vmareaObj;

  vmmap_insert(map, vmareaObj);
  return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{



	uint32_t bCount = 0;
	vmarea_t* vmareaObj = NULL;
	uint32_t finishAddr = 0;
	finishAddr = lopage + npages;

	list_iterate_begin(&map->vmm_list, vmareaObj, vmarea_t, vma_plink)
	{
		if((finishAddr < vmareaObj->vma_end) && (lopage > vmareaObj->vma_start))
		{

			vmarea_t* vmareaNew = vmarea_alloc();

			list_link_init(&vmareaNew->vma_plink);
			list_link_init(&vmareaNew->vma_olink);

			vmareaNew->vma_obj = vmareaObj->vma_obj;
			vmareaNew->vma_flags = vmareaObj->vma_flags;
			vmareaNew->vma_prot = vmareaObj->vma_prot;







			vmareaNew->vma_end = vmareaObj->vma_end;
			vmareaNew->vma_start = finishAddr;
			vmareaObj->vma_end = lopage;
			vmareaNew->vma_off = vmareaObj->vma_off + finishAddr - vmareaObj->vma_start;

			vmmap_insert(map,vmareaNew);

			list_insert_tail(mmobj_bottom_vmas(vmareaNew->vma_obj), &vmareaNew->vma_olink);


			if(vmareaObj->vma_obj->mmo_ops->ref != NULL){
				vmareaObj->vma_obj->mmo_ops->ref(vmareaObj->vma_obj);
			}

			bCount =finishAddr + bCount   -lopage;
			pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(finishAddr));
			return bCount;
		}
		else if((lopage < vmareaObj->vma_end  ) && lopage > (vmareaObj->vma_start) )
		{

			pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(vmareaObj->vma_end));
			vmareaObj->vma_end = lopage;
			bCount =  bCount - lopage + vmareaObj->vma_end;

		}
		else if((finishAddr < vmareaObj->vma_end  ) && (finishAddr > (vmareaObj->vma_start)))
		{

			pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vmareaObj->vma_start), (uintptr_t)PN_TO_ADDR(finishAddr));
			vmareaObj->vma_off = vmareaObj->vma_off + finishAddr-vmareaObj->vma_start;
			vmareaObj->vma_start = finishAddr;
			bCount = 0 - vmareaObj->vma_start + finishAddr + bCount  ;

			return bCount;
		}
		else if((finishAddr >= vmareaObj->vma_end) && (lopage <= vmareaObj->vma_start)  )
		{

			list_remove(&vmareaObj->vma_olink);
			if(vmareaObj->vma_obj->mmo_ops->put != NULL)
			{
				vmareaObj->vma_obj->mmo_ops->put(vmareaObj->vma_obj);
			}
			list_remove(&vmareaObj->vma_plink);
			pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vmareaObj->vma_start), (uintptr_t)PN_TO_ADDR(vmareaObj->vma_end));
			vmarea_free(vmareaObj);
			bCount = bCount - vmareaObj->vma_start + vmareaObj->vma_end ;
		}
		else
			continue;

	}list_iterate_end();


	return bCount;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
 
	uint32_t vfnMemLow = ADDR_TO_PN(USER_MEM_LOW);

  uint32_t endvfn = startvfn + npages;


  KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
  dbg(DBG_ALL, "GRADING3 1.e: startvfn is less than endvfn and USER_MEM_LOW is greater than startvfn and end vfn is less than USER_MEM_HIGH \n");

  vmarea_t* iterator = NULL;
  list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink)
    {
      if((iterator->vma_start >= endvfn)  && (  vfnMemLow <= startvfn) )
	{
	  return 1;
	}
      else if( (iterator->vma_start > startvfn) && ( iterator->vma_start < endvfn) )
      	{
    	  return 0;
      	}
      else if(vfnMemLow > startvfn)
	{
    	  return 0;
	}

			  
      vfnMemLow = iterator->vma_end;
			  
    }list_iterate_end();

  if( (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn) && (vfnMemLow <= startvfn)  )
    {
	  return 1;
    }
			

  return 0;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{


  uint32_t vfnInitial = ADDR_TO_PN(vaddr);


		
  void* convertedVaddr = (void*)vaddr;
  uint32_t vfnFinal = 1 + ADDR_TO_PN((uint32_t)vaddr + count);
  pframe_t* tempPgFrame = NULL;
  vmarea_t* iterator = NULL;
  uint32_t counter=0;
  uint32_t initialAddr = 0 ;
 	  uint32_t finalAddr = 0;
  	  uint32_t pgofset =0 ;
  list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink)
    {
      if(iterator->vma_end <= vfnInitial)
	continue;
      else if(iterator->vma_start >= vfnFinal )
	{
	  break;
	}
      else
	{
			  	
	   initialAddr = 0 ;
	   finalAddr = 0;
	  if(iterator->vma_start <= vfnInitial)
		  {
		  initialAddr = vfnInitial;
		  }
	  else
		  {
		  initialAddr = iterator->vma_start;
		  }


	  if(iterator->vma_end > vfnFinal )
		  {
		  finalAddr = vfnFinal;
		  }
	  else
		  {
		  finalAddr = iterator->vma_end;
		  }



	  tempPgFrame = NULL;
	  counter=0;
	  pgofset =0 ;
	  int temp = 0;
	  for(counter= initialAddr; counter < finalAddr;counter++)
	    {

	      pframe_get(iterator->vma_obj, iterator->vma_off + counter - iterator->vma_start , &tempPgFrame);

	      pgofset = ((uint32_t)convertedVaddr)%PAGE_SIZE;
	      convertedVaddr = PN_TO_ADDR(1 + counter);
						  
	      buf = (void *)(temp + (uint32_t)buf );
	      memcpy( buf, (void*)(pgofset + (uint32_t)(tempPgFrame->pf_addr)), MIN((0 -temp + count), (0 - pgofset + PAGE_SIZE)));
	      temp = temp + MIN(0 -temp + count,0 -pgofset + PAGE_SIZE);
	    }
			  	  
	}
    }list_iterate_end();
  return 0;

}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{

	uint32_t vfnInitial = ADDR_TO_PN(vaddr);
	void* tempVaddr = vaddr;
	int temp = 0;
	uint32_t vfnFinal = 1 +  ADDR_TO_PN((uint32_t)vaddr + count);
	pframe_t* tempPgFrame = NULL;
	uint32_t initialAddr = 0 ;
				uint32_t finalAddr = 0 ;
				uint32_t counter=0;
	vmarea_t* iterator = NULL;
	list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink)
	{
		if(iterator->vma_end <= vfnInitial)
		{
			continue;
		}
		else if(iterator->vma_start >= vfnFinal)
		{
			break;
		}
		else
		{
			initialAddr = 0 ;
			finalAddr = 0 ;

			if(iterator->vma_start <= vfnInitial)
			{
				initialAddr = vfnInitial;
			}
			else
			{

				initialAddr = iterator->vma_start;
			}

			if(iterator->vma_end > vfnFinal )
				{
				finalAddr = vfnFinal;
				}
			else
				{

				finalAddr = iterator->vma_end;
				}



			 counter=0;
			 uint32_t pgOfset = 0;
			 tempPgFrame = NULL;
			for(counter= initialAddr; counter < finalAddr;counter++)
			{

				pframe_get(iterator->vma_obj, (iterator->vma_off - iterator->vma_start + counter ) , &tempPgFrame);

				pgOfset= ((uint32_t)tempVaddr)%PAGE_SIZE;
				tempVaddr = PN_TO_ADDR(1 + counter);

				buf = (void*) (temp + (uint32_t)buf);
				pframe_dirty(tempPgFrame);
				pframe_set_busy(tempPgFrame);

				memcpy( (void*)((uint32_t)(tempPgFrame->pf_addr)+pgOfset),buf, MIN(0-temp + count, (0 -pgOfset + PAGE_SIZE)));
				pframe_clear_busy(tempPgFrame);
				sched_broadcast_on(&tempPgFrame->pf_waitq);
				temp = temp +  MIN(0 -temp + count, 0 -pgOfset + PAGE_SIZE);
			}

		}
	}list_iterate_end();
	return 0;
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
  KASSERT(0 < osize);
  KASSERT(NULL != buf);
  KASSERT(NULL != vmmap);

  vmmap_t *map = (vmmap_t *)vmmap;
  vmarea_t *vma;
  ssize_t size = (ssize_t)osize;

  int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
		     "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
		     "VFN RANGE");
  /*dbg(DBG_CORE,"%21s %5s %7s %8s %10s %12s\n",
    "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
    "VFN RANGE");*/

  list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
    size -= len;
    buf += len;
    if (0 >= size) {
      goto end;
    }

    len = snprintf(buf, size,
		   "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
		   vma->vma_start << PAGE_SHIFT,
		   vma->vma_end << PAGE_SHIFT,
		   (vma->vma_prot & PROT_READ ? 'r' : '-'),
		   (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
		   (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
		   (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
		   vma->vma_obj/*,vma->vma_obj->mmo_un.mmo_bottom_obj*/, vma->vma_off, vma->vma_start, vma->vma_end);
				
    /*dbg(DBG_CORE,"%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
      vma->vma_start << PAGE_SHIFT,
      vma->vma_end << PAGE_SHIFT,
      (vma->vma_prot & PROT_READ ? 'r' : '-'),
      (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
      (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
      (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
      vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);*/
  } list_iterate_end();

 end:
  if (size <= 0) {
    size = osize;
    buf[osize - 1] = '\0';
  }
  /*
    KASSERT(0 <= size);
    if (0 == size) {
    size++;
    buf--;
    buf[0] = '\0';
    }
  */
  return osize - size;
}
