#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int  shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int  shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int  shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
		.ref = shadow_ref,
		.put = shadow_put,
		.lookuppage = shadow_lookuppage,
		.fillpage  = shadow_fillpage,
		.dirtypage = shadow_dirtypage,
		.cleanpage = shadow_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void
shadow_init()
{

	shadow_allocator = slab_allocator_create("ShadowObject",sizeof(mmobj_t));
	KASSERT(shadow_allocator);
	dbg(DBG_ALL, "GRADING3 3.a: shadow_allocator is not NULL \n");

}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create()
{
	mmobj_t* s_mmobj = slab_obj_alloc(shadow_allocator);
	mmobj_init(s_mmobj, &shadow_mmobj_ops);
	s_mmobj->mmo_refcount = 1;
	return s_mmobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{

	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
	dbg(DBG_ALL, "GRADING3 3.b: mmobj_t object is not NULL and its ref count is greater than zero and shadow_mmobj_ops is equal to object's mmo_ops \n");
	o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void
shadow_put(mmobj_t *o)
{
	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));

	dbg(DBG_ALL, "GRADING3 3.c: mmobj_t object is not NULL and its ref count is greater than zero and shadow_mmobj_ops is equal to object's mmo_ops \n");
	o->mmo_refcount = o->mmo_refcount -1;
	static uint32_t num = 0;
	pframe_t* tempFrm = NULL;

	if( o->mmo_nrespages == o->mmo_refcount){

		list_t* tempList = o->mmo_respages.l_next;


		for(; &o->mmo_respages != tempList; )
		{

			tempFrm = list_item(tempList, pframe_t, pf_olink);
			num = num + 1;
			pframe_unpin(tempFrm);
			while (pframe_is_busy(tempFrm))
				{
				sched_sleep_on(&(tempFrm->pf_waitq));
				}


			num = num -1;
			pframe_free(tempFrm);
			tempList = o->mmo_respages.l_next;
		}

		if(0 == num){
			/*Check here*/
		}else{
			o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
			slab_obj_free( shadow_allocator, o);
		}
	}
}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
	pframe_t* tpframe = NULL;

	mmobj_t* tempObj = o;
	pframe_t* spframe = NULL;
	uint32_t flag = 0;
	while(NULL != tempObj->mmo_shadowed){
		list_iterate_begin(&tempObj->mmo_respages, tpframe, pframe_t, pf_olink) {
			if(  pagenum == tpframe->pf_pagenum){

				flag = 1;
				spframe = tpframe;
				break;
			}
		} list_iterate_end();

		if(0 != flag)
			break;
		tempObj = tempObj->mmo_shadowed;
	}

	if(0 == flag){
		pframe_get(o->mmo_un.mmo_bottom_obj, pagenum, &spframe);
	}

	if(0 == forwrite){
		*pf = spframe;
		return 0;
			}
	else{


		if(o != spframe->pf_obj){

					pframe_get(o, pagenum, pf);
					return 0;

				}
				else{

					*pf = spframe;
								return 0;
				}


	}
}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain). */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
	KASSERT(pframe_is_busy(pf));
	dbg(DBG_ALL, "GRADING3 3.d: pframe is busy \n");

	KASSERT(!pframe_is_pinned(pf));
	dbg(DBG_ALL, "GRADING3 3.f: pframe is pinned \n");

	pframe_pin(pf);

	mmobj_t* tempObj = o->mmo_shadowed;
	int flag = 0;
	pframe_t* tpframe;
	pframe_t* npframe;

	while(NULL != tempObj->mmo_shadowed){
		list_iterate_begin(&tempObj->mmo_respages, tpframe, pframe_t, pf_olink) {
			if( pf->pf_pagenum == tpframe->pf_pagenum ){

				flag = 1;
				npframe = tpframe;
				break;
			}
		} list_iterate_end();

		if(1 == flag)
			break;
		tempObj = tempObj->mmo_shadowed;
	}
	if(0 == flag){
		pframe_get(tempObj, pf->pf_pagenum, &npframe);
	}
	memcpy(pf->pf_addr, npframe->pf_addr, PAGE_SIZE);
	return 0;
}
/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
	return 0;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
	/*KASSERT(1);*/
	return 0;
}

