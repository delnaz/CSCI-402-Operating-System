#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
  const char *cur = ".";
  const char *par = "..";
  
  KASSERT(name);
  KASSERT(dir);
  KASSERT(result);
  /*  dbg(DBG_CORE,"Checking inside Inode %x with number %d for file %s\n",dir,dir->vn_vno,name);*/
  /*if its a dir or not*/
  if(!dir->vn_ops->lookup)
    return -ENOTDIR;
  
  if(!strncmp(cur, name, len))
    {
      *result = dir;
      vget((*result)->vn_fs, (*result)->vn_vno);
      return 0;
    }
  /*Parent .. Taken care inside lookup*/  
  int res = dir->vn_ops->lookup(dir, name, len, result);
  return res;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
    const char separator = '/';
    
  vnode_t* tempVnode = NULL;
  
  
  
  
  
  KASSERT(pathname);
  KASSERT(res_vnode);
  KASSERT(namelen);
  KASSERT(name);

  char localPathArr[MAXPATHLEN];
  strcpy(localPathArr,pathname);
   
  if(NULL == base)
    base = curproc->p_cwd;
  
  int i = 0;
  if(pathname[i] != '/')
    {
          tempVnode = base;
      
    }
  else
    {
          i++;
                tempVnode = vfs_root_vn;    
    }
        
  vget(tempVnode->vn_fs,tempVnode->vn_vno);
  
  
  
  if(localPathArr[i] == '\0')
    {
      if(0 == i)
        {
          vput(tempVnode);
          return -EINVAL;
        }
      else
        {
              *res_vnode = tempVnode;
              *namelen = strlen(pathname);
          *name = pathname;
          
          
          return 0;
        }
    }
  char* localPtr  = &localPathArr[i];
  
  char *tempRes = strchr( localPtr, separator );
  if(NULL != tempRes)
    *tempRes++ = '\0';
  

  if(0 < strlen(localPtr))
    {
          *namelen = strlen(localPtr);
      if( NAME_LEN < *namelen)
        {
          vput(tempVnode);
          return -ENAMETOOLONG;
        }
    }
  vnode_t* vnodeObj = NULL;
  int res =0;
  for( ;NULL != localPtr ; ) 
    {
         vnodeObj = NULL;
      res = lookup(tempVnode,localPtr, strlen(localPtr), &vnodeObj);
      localPtr = tempRes;    
      if(NULL == localPtr)
        {

              if(0 <= res)
                {
                  vput(vnodeObj);
                }                   
              if((0 > res) && (-ENOENT != res))
                {
                  vput(tempVnode);
                  return res;
                }
              
        }
      else
        {       
          
          
          if(localPtr == '\0')
            {
              if( 0 > res)
                {

                      vput(tempVnode);
                      return res;
                }
                        
              else if( -ENOENT != res)
                {
                  
                  vput(vnodeObj);
                }

              break;
            }
          tempRes = strchr( localPtr, separator );
          if(NULL != tempRes)
            *tempRes++ = '\0';
          if(0 > res )
            {
              vput(tempVnode);
              return res;
            }
          *namelen = strlen(localPtr);
          if(NAME_LEN <  *namelen  )
            {
                  vput(vnodeObj);
              vput(tempVnode);
              
              return -ENAMETOOLONG;
            }
          vput(tempVnode);
          tempVnode = vnodeObj;
          
        }
        
    }
  
  *res_vnode =  tempVnode;
  KASSERT(*res_vnode);
  *name = pathname+strlen(pathname)-*namelen;
  
 
  return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fnctl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
    const char *thiDir = ".";
  const char* file_naav;
  const char* slash = "/";
  
  vnode_t* tempNode = NULL;
  size_t length=0;
  int res = dir_namev( pathname, &length, &file_naav, base, &tempNode);
  
  if(0 > res )
    {
      return res;
    }

  if(!strncmp(file_naav, slash, length) && (vfs_root_vn == tempNode))
    file_naav =  thiDir;
  res = lookup( tempNode, file_naav, length, res_vnode);
  if(0 > res )
    {
      if(!((O_CREAT & flag )&&(-ENOENT == res)))
        {
              vput(tempNode);
                        return res;
                        
        }
      else
        {
          
          KASSERT(tempNode->vn_ops->create);   
                    res = tempNode->vn_ops->create(tempNode,file_naav,length,res_vnode);
        }
    }
    
  vput(tempNode);
  
  return res;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
  NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
  return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
  NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

  return -ENOENT;
}
#endif /* __GETCWD__ */
