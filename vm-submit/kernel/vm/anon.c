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

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
  .ref = anon_ref,
  .put = anon_put,
  .lookuppage = anon_lookuppage,
  .fillpage  = anon_fillpage,
  .dirtypage = anon_dirtypage,
  .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{
  anon_allocator = slab_allocator_create("anon obj", sizeof(mmobj_t));
  KASSERT(anon_allocator);
  dbg(DBG_ALL, "GRADING3 2.a: anon_allocator is not NULL \n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{
  
  mmobj_t* mmobj_temp =(mmobj_t*) slab_obj_alloc(anon_allocator);
  if(NULL == mmobj_temp)
    return NULL;
  

  mmobj_init(mmobj_temp, &anon_mmobj_ops);
  mmobj_temp->mmo_refcount = 1;  
  
  return mmobj_temp;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{

  
  KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
  dbg(DBG_ALL, "GRADING3 2.b: mmobj_t is not null and ref count of obj is greater than zero and anon_mmobj_ops equals to obj->mmo_ops\n");

  o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{


  KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
  dbg(DBG_ALL, "GRADING3 2.c: mmobj_t is not null and ref count of obj is greater than zero and anon_mmobj_ops equals to obj->mmo_ops\n");

  
  
  static uint32_t temp_c = 0; /*counter*/
  
  pframe_t* temp_pframe = NULL;
  list_t* list_obj = NULL;
  o->mmo_refcount = o->mmo_refcount -1;
  
  

  if(o->mmo_nrespages == o->mmo_refcount)
    {
      temp_pframe = NULL;       
      list_obj = o->mmo_respages.l_next;
                  
                
      for(;&o->mmo_respages != list_obj ;)
        {
          
          
          temp_pframe = list_item(list_obj, pframe_t, pf_olink);
          pframe_unpin(temp_pframe);
          temp_c = temp_c + 1;
          for (;1 == pframe_is_busy(temp_pframe);)
            sched_sleep_on(&(temp_pframe->pf_waitq));

          pframe_free(temp_pframe);
          temp_c = temp_c - 1;
          list_obj = o->mmo_respages.l_next;
        }

      if(0 == temp_c)
        slab_obj_free( anon_allocator, o);      
    }
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
  return pframe_get(o, pagenum, pf);
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
  KASSERT(pframe_is_busy(pf));
  dbg(DBG_ALL, "GRADING3 2.d: pframe is busy \n");


  KASSERT(!pframe_is_pinned(pf));
  dbg(DBG_ALL, "GRADING3 2.d: pframe is pinned \n");
                
  pframe_pin(pf);
  memset(pf->pf_addr, 0, PAGE_SIZE);
  
  return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
 
  return -1;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
 
  
  return -1;
}
