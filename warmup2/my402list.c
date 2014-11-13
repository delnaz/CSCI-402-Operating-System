#include<stdio.h>
#include<stdlib.h>
#include"my402list.h"

int  My402ListLength(My402List* list)
{

	return list->num_members;
}

int  My402ListEmpty(My402List* list)
{
	if(list->num_members==0)
	return TRUE;
	else
	return FALSE;
}

int My402ListInit(My402List* list)
{
	list->num_members=0;
	list->anchor.obj=NULL;	
	list->anchor.next=&(list->anchor);
	list->anchor.prev=&(list->anchor);
	return TRUE;
}

My402ListElem *My402ListFirst(My402List* list)
{
	if(list->num_members==0)
	return NULL;
	return list->anchor.next;
}

My402ListElem *My402ListLast(My402List* list)
{
	if(list->num_members==0)
	return NULL;
	return list->anchor.prev;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* elem)
{
	if(elem->next==&(list->anchor))
	return NULL;
	return elem->next;

}
My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem)
{
	if(elem->prev==&(list->anchor))
	return NULL;
	return elem->prev;

}


int  My402ListAppend(My402List* list, void* obj)
{
	My402ListElem* newlist= (My402ListElem *)malloc(sizeof(My402ListElem));
	if(newlist==NULL)
	{
		return FALSE;
	}
	list->num_members=list->num_members+1;
	newlist->obj=obj;
	newlist->next=&(list->anchor);
	newlist->prev=list->anchor.prev;
	list->anchor.prev=newlist;
	newlist->prev->next=newlist;	
	
	return TRUE;
}

int  My402ListPrepend(My402List* list, void* obj)
{
	My402ListElem* newlist= (My402ListElem*) malloc(sizeof(My402ListElem));
	if(newlist==NULL)
	{
		return FALSE;
	}
	list->num_members=list->num_members+1;
	newlist->obj=obj;
	newlist->next=list->anchor.next;
	newlist->prev=&(list->anchor);
	list->anchor.next=newlist;
	newlist->next->prev=newlist;	
	
	return TRUE;	

}
void My402ListUnlink(My402List* list, My402ListElem* elem)
{
	elem->prev->next=elem->next;
	elem->next->prev= elem->prev;
	elem->next=NULL;
	elem->prev=NULL;
	elem->obj=NULL;
	free(elem);
	list->num_members=list->num_members-1;

}

void My402ListUnlinkAll(My402List* list)
{

	My402ListElem* elem=list->anchor.next;
	My402ListElem* temp=NULL;
	for(temp=elem->next;elem!=&(list->anchor);temp=temp->next)
	{
		elem->next=NULL;
		elem->prev=NULL;
		elem->obj=NULL;
		free(elem);
		elem=temp;
	}
	My402ListInit(list);
}

int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* elem)
{

	if(elem==NULL)
	{
		return My402ListAppend(list,obj);
	}
	My402ListElem* newnode=(My402ListElem*) malloc(sizeof(My402ListElem));
	if(newnode==NULL)
	{
		return FALSE;	
	}
	list->num_members=list->num_members+1;
	newnode->obj=obj;
	newnode->next=elem->next;
	newnode->prev=elem->next->prev;
	elem->next->prev=newnode;
	elem->next=newnode;
	return TRUE;

}

int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* elem)
{
	if(elem==NULL)
	{
		return My402ListPrepend(list,obj);
	}
	My402ListElem* newnode=(My402ListElem*) malloc(sizeof(My402ListElem));
	if(newnode==NULL)
	{
		return FALSE;
	}
	list->num_members=list->num_members+1;
	newnode->obj=obj;
	newnode->next=elem;
	newnode->prev=elem->prev;
	elem->prev->next=newnode;
	elem->prev=newnode;
	return TRUE;
}


My402ListElem *My402ListFind(My402List* list, void* obj)
{
	My402ListElem* elem=NULL;
	int length= My402ListLength(list);
	for(elem=My402ListFirst(list); length!=0; elem=elem->next)
	{
	if(elem->obj==obj)
		return elem;
		length--;
	}
	return NULL;

}

