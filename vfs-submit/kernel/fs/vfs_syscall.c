/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.2 2013/11/02 15:53:00 william Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read fs_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_read");*/
	
	if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL)
	{
		dbg(DBG_ERROR,"\n do_read():fd is not an open file descriptor.\n");
     	return -EBADF;
	}
	
	file_t * file = fget(fd);
	if(file == NULL)
	{
		dbg(DBG_ERROR, "\ndo_read(): fd is not a valid file descriptor\n");
		return -EBADF;
	}

	if( (file->f_mode & FMODE_READ) != FMODE_READ )
	{	
		dbg(DBG_ERROR, "\ndo_read(): fd is not open for reading\n");	
		fput(file);
		return -EBADF;
	}

	if (S_ISDIR(file->f_vnode->vn_mode))
	{
		dbg(DBG_ERROR, "\ndo_read(): This is a directory\n");
		fput(file);
		return -EISDIR;
	}
	
	int result = 0;
	result = file->f_vnode->vn_ops->read(file->f_vnode,file->f_pos,buf,nbytes);
	file->f_pos = file->f_pos + result;
	fput(file);
	return result;
}
/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_write");*/
	
	if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL)
	{
		dbg(DBG_ERROR,"\ndo_write(): fd is not an open file descriptor.\n"); 
		return -EBADF;
	}

	file_t *file = fget(fd);

	if(file == NULL)
	{
		dbg(DBG_ERROR, "\ndo_write(): fd is not a valid file descriptor or is not open for reading\n");
		return -EBADF;
	}
	
	if((file->f_mode & FMODE_WRITE) != FMODE_WRITE ) {
		fput(file);
		dbg(DBG_ERROR,"\ndo_write(): fd is not open for writing.\n");
		
		return -EBADF;
	}
	
	if((file->f_mode & FMODE_APPEND) == FMODE_APPEND) {
		file->f_pos = do_lseek(fd,0,SEEK_END);
	}

	int result1 = 0;
	result1 = file->f_vnode->vn_ops->write(file->f_vnode, file->f_pos,buf,nbytes);
	if(!(result1<0))
	{
	 	KASSERT((S_ISCHR(file->f_vnode->vn_mode)) || (S_ISBLK(file->f_vnode->vn_mode)) || ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len)));
	 	dbg(DBG_PRINT,"\n(GRADING2A 3.a) A character special CHR or block BLK or REG write is successful \n");
		file->f_pos=file->f_pos+result1;
	}
	fput(file);
	return result1;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_close");*/
	if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL) {
		dbg(DBG_ERROR,"\ndo_close(): fd is not an open file descriptor.\n"); 
		return -EBADF;
	}
	fput(curproc->p_files[fd]);
	curproc->p_files[fd] = NULL;
	return 0;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{

    if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL) {
		dbg(DBG_ERROR,"\ndo_dup(): fd is not an open file descriptor.\n"); 
		return -EBADF;
    }

    file_t *file = fget(fd);

    if(file == NULL)
    {
		dbg(DBG_ERROR, "\ndo_dup(): fd is not a valid file descriptor or is not open for reading\n");
		return -EBADF;
    }

    int fd1 = get_empty_fd(curproc);
    if(fd1 < 0)
    {    
	dbg(DBG_ERROR, "\ndo_dup(): The process already has the maximum number of file descriptors open and tried to open a new one\n");
	fput(file);
        return fd1;
    }

    curproc->p_files[fd1] = file;
    return fd1;
    /* NOT_YET_IMPLEMENTED("VFS: do_dup");
    return -1; */
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
	/* //NOT_YET_IMPLEMENTED("VFS: do_dup2");
        //return -1; */

	if(ofd<0 || ofd >= NFILES || curproc->p_files[ofd] == NULL){
		dbg(DBG_ERROR,"\ndo_dup2(): ofd is not an open file descriptor.\n"); 
		return -EBADF;
        }
	if(nfd<0 || nfd >= NFILES){
		dbg(DBG_ERROR,"\ndo_dup2(): nfd is not an open file descriptor.\n"); 
		return -EBADF;
        }

    	file_t *file = fget(ofd);
	
	if(file == NULL)
    	{
		dbg(DBG_ERROR, "\ndo_dup2(): fd is not a valid file descriptor or is not open for reading\n");
		return -EBADF;
    	}
	
	if (nfd == ofd) {
		fput(file);
		return nfd;
	}
	int ret = 0;
    	if(fget(nfd)!=NULL && ofd!=nfd) {
              if((ret = do_close(nfd)) < 0)
	      {
		  fput(file);
		  return ret;
	      }
	}
    	
    	curproc->p_files[nfd] = file;
   
    	return nfd;
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */

int
do_mknod(const char *path, int mode, unsigned devid)
{
        /*-----------------------------------------------------------------------------------------------------------------*/
   if(mode != S_IFCHR && mode != S_IFBLK)
     {
       dbg(DBG_ERROR, "\ndo_mknod(): Wrong mode.");
           return -EINVAL;
     }
   if(strlen(path) > MAXPATHLEN)
     {
       dbg(DBG_ERROR, "\ndo_mknod(): A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if( strlen(path) <= 0)
     {
       dbg(DBG_ERROR, "\ndo_mknod(): Wrong Path.");
           return -EINVAL;
     }

   const char *name = NULL;
   size_t namelen = 0;
   vnode_t *res_vnode = NULL;
   int value = 0;
   value = dir_namev(path, &namelen, &name, NULL, &res_vnode);
   if(value < 0)
     {
	dbg(DBG_ERROR, "\ndo_mknod(): error in dir_namev call.");
      return value;
     }
    	vput(res_vnode);

   int value_lookup = 0;
   value_lookup = lookup(res_vnode, name, namelen, &res_vnode);

   if(value_lookup == 0)
          {
           dbg(DBG_ERROR, "\ndo_mknod(): path already exists.");
	   vput(res_vnode);
           return -EEXIST;
          }
   else if(value_lookup == -ENOENT)
        {
          KASSERT(NULL != res_vnode->vn_ops->mknod);
          dbg(DBG_PRINT,"\n(GRADING2A 3.b) Corresponding vn_ops not NULL");
          int value_mknod = 0;
	value_mknod = res_vnode->vn_ops->mknod(res_vnode, name, namelen, mode, devid);
          return value_mknod;
        } 
   else
       return value_lookup;

return -1;

 /*-----------------------------------------------------------------------------------------------------------------*/
        /*NOT_YET_IMPLEMENTED("VFS: do_mknod");
        return -1;*/
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
        /*-----------------------------------------------------------------------------------------------------------------*/
   if(strlen(path) > MAXPATHLEN)
     {
       dbg(DBG_ERROR, "\ndo_mkdir(): A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if( strlen(path) <= 0)
     {
       dbg(DBG_ERROR, "\ndo_mkdir(): Wrong Path.");
           return -EINVAL;
     }
   const char *name = NULL;
   size_t namelen = 0;
   vnode_t *res_vnode;
	
   int value = 0;
	value = dir_namev(path, &namelen, &name, NULL, &res_vnode);
   if(value < 0)
     {
	dbg(DBG_ERROR, "\ndo_mkdir(): error in dir_namev call.");
      return value;
     }

	vput(res_vnode);

   int value_lookup = 0;
	value_lookup = lookup(res_vnode, name, namelen, &res_vnode);
   if(value_lookup == -ENOTDIR)
     {
      dbg(DBG_ERROR, "\ndo_mkdir(): A component used as a directory in path is not, in fact, a directory.");
      return -ENOTDIR;
     }
   else if(value_lookup == 0)
          {
           dbg(DBG_ERROR, "\ndo_mkdir(): path already exists.");
	   vput(res_vnode);
           return -EEXIST;
          }
   else
     {
      if(value_lookup == -ENOENT)
        {
          KASSERT(NULL != res_vnode->vn_ops->mkdir);
          dbg(DBG_PRINT,"\n(GRADING2A 3.c) Corresponding vn_ops not NULL");
          int value_mkdir = 0;
	value_mkdir = res_vnode->vn_ops->mkdir(res_vnode, name, namelen);
          return value_mkdir;
        }
  
    }
   return value_lookup;

 /*-----------------------------------------------------------------------------------------------------------------*/
        /*NOT_YET_IMPLEMENTED("VFS: do_mkdir");
        return -1;*/
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_rmdir(const char *path)
{
        /*-----------------------------------------------------------------------------------------------------------------*/
   if(strlen(path) > MAXPATHLEN)
     {
       dbg(DBG_ERROR, "\ndo_rmdir(): A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if( strlen(path) <= 0)
     {
       dbg(DBG_ERROR, "\ndo_rmdir(): Wrong Path.");
           return -EINVAL;
     }

  
   const char *name = NULL;
   size_t namelen = 0;
   vnode_t *res_vnode;
   int value = 0;

   value = dir_namev(path, &namelen, &name, NULL, &res_vnode);

   if(value < 0)
     {
	dbg(DBG_ERROR, "\ndo_rmdir(): error in dir_namev call.");
      return value;
     }

   if(name_match(".", name, namelen))
     {
	vput(res_vnode);
      dbg(DBG_ERROR, "\ndo_rmdir(): path has \".\" as its final component.");
      return -EINVAL;
     }
   if(name_match("..", name, namelen))
     {
	vput(res_vnode);
      dbg(DBG_ERROR, "\ndo_rmdir(): path has \"..\" as its final component.");
      return -ENOTEMPTY;
     }

   KASSERT(NULL != res_vnode->vn_ops->rmdir);
   dbg(DBG_PRINT,"\n(GRADING2A 3.d) Corresponding vn_ops not NULL");
   	
   int value_rmdir = 0;
  value_rmdir = res_vnode->vn_ops->rmdir(res_vnode, name, namelen);
	vput(res_vnode);
	if (value_rmdir < 0)
	{
		return value_rmdir;
	}

   return 0;
/*-----------------------------------------------------------------------------------------------------------------*/
        /*NOT_YET_IMPLEMENTED("VFS: do_rmdir");
        return -1;*/
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
        if(strlen(path) > MAXPATHLEN)
     {
       dbg(DBG_ERROR, "\ndo_unlink(): A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if( strlen(path) <= 0)
     {
       dbg(DBG_ERROR, "\ndo_unlink(): Wrong Path.");
           return -EINVAL;
     }
  
   const char *name = NULL;
   size_t namelen = 0;
   vnode_t *res_vnode = NULL, *result_vnode = NULL;
   int value = 0;
	value = dir_namev(path, &namelen, &name, NULL, &res_vnode);
   if(value < 0)
     {
	dbg(DBG_ERROR, "\ndo_unlink(): error in dir_namev call.");
        return value;
     }

	value = lookup(res_vnode, name, namelen, &result_vnode);
     if(value < 0)
     {
	dbg(DBG_ERROR, "\ndo_unlink(): error in lookup call.");
	vput(res_vnode);
        return value;
     }


   if(S_ISDIR(result_vnode->vn_mode))
     {
       vput(res_vnode);
	vput(result_vnode);
	dbg(DBG_ERROR, "\ndo_unlink(): path is directory.");
       return -EISDIR;
     }

   KASSERT(NULL != res_vnode->vn_ops->unlink);
   dbg(DBG_PRINT,"\n(GRADING2A 3.e) do_unlink(): Corresponding vn_ops not NULL");
  
   int value_ulink = 0;
	 value_ulink = res_vnode->vn_ops->unlink(res_vnode, name, namelen);
	vput(result_vnode);
	vput(res_vnode);
   return value_ulink;
        /*NOT_YET_IMPLEMENTED("VFS: do_unlink");
        return -1;*/
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 */
int
do_link(const char *from, const char *to)
{
        if(strlen(from) > MAXPATHLEN)
     {
       dbg(DBG_ERROR, "\ndo_link(): A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if(strlen(from) <= 0)
     {
       dbg(DBG_ERROR, "\ndo_link(): Wrong Path.");
           return -EINVAL;
     }
   if(strlen(to) > MAXPATHLEN)
     {
       dbg(DBG_ERROR, "\ndo_link(): A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if(strlen(to) <= 0)
     {
       dbg(DBG_ERROR, "\ndo_link(): Wrong Path.");
           return -EINVAL;
     }
   
    vnode_t *res_vnode1;
    int value_opennamev = open_namev(from, 0, &res_vnode1, NULL);

    if(value_opennamev < 0)
       {
         return value_opennamev;
       }
    if(S_ISDIR(res_vnode1->vn_mode))
     {
	dbg(DBG_ERROR, "\ndo_link(): From can not be directory.");
       return -EISDIR;
     }

    const char *name= NULL;
    size_t namelen = 0;
    vnode_t *res_vnode2;

    int value_dirnamev= dir_namev(to, &namelen, &name, NULL, &res_vnode2);
	vput(res_vnode1);
    if(value_dirnamev < 0)
       {
	
         return value_dirnamev;
       }

     vput(res_vnode2);

	int value_link = lookup(res_vnode2, name, namelen, &res_vnode2);
	vput(res_vnode1);
	if(value_link == 0)
	{
		vput(res_vnode2);
		dbg(DBG_ERROR, "\ndo_link(): Name already exists\n");
		return -EEXIST;
	}
	else if(value_link == -ENOENT)
	{
		value_link = res_vnode2->vn_ops->link(res_vnode1, res_vnode2, name, namelen);
		return value_link;
	}
	else
	{
		
		dbg(DBG_ERROR, "\ndo_link(): Error while returning from lookup()\n");
		return value_link;
	}

        /*NOT_YET_IMPLEMENTED("VFS: do_link");*/
        return -1;

        /*NOT_YET_IMPLEMENTED("VFS: do_link");
        return -1;*/
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_rename");*/
		if(strlen(oldname) < 1 || strlen(newname) < 1)
	{
		dbg(DBG_ERROR,"\ndo_rename() : Returns due to invalid argument\n");
		return -EINVAL;
	}
        do_link(oldname,newname);
	return do_unlink(oldname);
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
        /*---------------------------------------------------------------------------------------------------------------------*/
 if(strlen(path) > MAXPATHLEN)
     {
       dbg(DBG_PRINT, "\ndo_chdir() : A component of path was too long.");
             return -ENAMETOOLONG;
     }
   if(strlen(path) <= 0)
     {
       dbg(DBG_PRINT, "\ndo_chdir() : Wrong Path.");
           return -EINVAL;
     }

   vnode_t *res_vnode;
   int value = open_namev(path, 0, &res_vnode, NULL);
   if(value < 0)
     {
      return value;
     }

   if(!S_ISDIR(res_vnode->vn_mode))
     {
       vput(res_vnode);
       return -ENOTDIR;
     }
	vput(curproc->p_cwd);
    curproc->p_cwd=res_vnode;
    return 0;
/*---------------------------------------------------------------------------------------------------------------------*/
        /*NOT_YET_IMPLEMENTED("VFS: do_chdir");
        return -1;*/
}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
 * If the readdir fs_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_getdent");*/
	
	if(fd < 0 || fd >= NFILES ||curproc->p_files[fd] == NULL) 
	{
		return -EBADF;
	}

	file_t *file = fget(fd);
	
	if(file == NULL)
	{	dbg(DBG_ERROR,"\ndo_getdent(): fd is not a valid file descriptor.\n");
		return -EBADF;
	}

	if (!(S_ISDIR(file->f_vnode->vn_mode)))
	{
		fput(file);
		return -ENOTDIR;
	}

	int result = file->f_vnode->vn_ops->readdir(file->f_vnode,file->f_pos,dirp);
	file->f_pos = file->f_pos + result;
	fput(file);
	if(result == 0)
	{
		return 0;
	}
		return sizeof(*dirp);
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_lseek");*/
	if(fd < 0 || fd >= NFILES) {
		return -EBADF;
	}
	if((whence != SEEK_SET) && (whence != SEEK_CUR) && (whence != SEEK_END)) {
		return -EINVAL;
	}

	file_t *file = fget(fd);

	if(file == NULL)
	{	dbg(DBG_ERROR,"\ndo_lseek(): fd is not a valid file descriptor.\n");
		return -EBADF;
	}

	int FileOffset = file->f_pos;
	
	
	if(whence == SEEK_SET) {
		FileOffset = offset;
	}
	else if(whence == SEEK_CUR) {
		FileOffset = FileOffset + offset;
	}
	else if(whence == SEEK_END) {
		FileOffset = (file->f_vnode->vn_len) + offset;
	}
	
	fput(file);
	if(FileOffset < 0) {
		file->f_pos = 0;
		return -EINVAL;
	}
	file->f_pos = FileOffset;
	
	return FileOffset;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_stat(const char *path, struct stat *buf)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_stat");*/
	if(strlen(path) > MAXPATHLEN)
	{
		dbg(DBG_PRINT, "\n A component of path was too long.");
		return -ENAMETOOLONG;
	}
	if(strlen(path) <= 0)
     {
       dbg(DBG_PRINT, "\ndo_stat() : Wrong Path.");
           return -EINVAL;
     }
	  
	const char *name = NULL;
	size_t namelen = 0;
	vnode_t *res_vnode;
	int value = dir_namev(path, &namelen, &name, NULL, &res_vnode);

	if(value < 0)
	{
		return value;
	}
	vput(res_vnode);

	value = lookup(res_vnode, name, namelen, &res_vnode);

	if(value < 0)
	{
		
		dbg(DBG_ERROR, "\ndo_stat(): Error while returning from lookup()\n");
		return -ENOENT;
	}
	KASSERT(NULL != res_vnode->vn_ops->stat);
	dbg(DBG_PRINT,"(GRADING2A 3.f) do_stat(): Function pointer stat exists");
	
	int r = res_vnode->vn_ops->stat(res_vnode,buf);
	vput(res_vnode);

	return r;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif
