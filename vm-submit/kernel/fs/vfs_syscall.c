/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.1 2012/10/10 20:06:46 william Exp $
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
 *      o call its virtual read f_op
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

        if( 0 > fd)
                return -EBADF;
        if(NFILES <= fd)
                return -EBADF;
        file_t * ftemp = fget(fd);

        if(NULL == ftemp)
        {
                
                return -EBADF;
        }
        if(!(ftemp->f_mode & FMODE_READ))
        {
                fput(ftemp);
                return -EBADF;  
        }

        if(S_ISDIR( ftemp-> f_vnode->vn_mode))
        {
                fput(ftemp);
                return -EISDIR;
        }


        KASSERT(ftemp->f_vnode->vn_ops->read != NULL && "File Read function pointer not set");

        int nretVal = ftemp->f_vnode->vn_ops->read(ftemp->f_vnode,ftemp->f_pos,buf,nbytes);
        nretVal > 0? ftemp->f_pos+=nretVal : NULL;
        fput(ftemp);
        return nretVal;

}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * f_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
        if(fd < 0)
                return -EBADF;
        if(fd>=NFILES)
                return -EBADF;

    
        file_t * ftemp = fget(fd);

        if (NULL == ftemp)
            {
                return -EBADF;
            }
        if(!(ftemp->f_mode & FMODE_WRITE))
        {
                fput(ftemp);
                return -EBADF;
        }


        KASSERT(ftemp->f_vnode->vn_ops->write != NULL && "function pointer not set");

        int pos=0;

        if(FMODE_APPEND & ftemp->f_mode )
                pos = do_lseek(fd ,0 , SEEK_END);
        else
                pos= ftemp->f_pos;


        int retVal= ftemp->f_vnode->vn_ops->write(ftemp->f_vnode, pos, buf, nbytes);
        if(retVal >= 0)
        {
                KASSERT((S_ISBLK(ftemp->f_vnode->vn_mode)) ||
                                (S_ISCHR(ftemp->f_vnode->vn_mode)) ||
                        ((S_ISREG(ftemp->f_vnode->vn_mode)) && (ftemp->f_pos <= ftemp->f_vnode->vn_len)));
                dbg(DBG_ALL, "(GRADING2 3.a):ftemp is one of the Character/Block/Regular device with current file pos less than file length.\n");
                ftemp->f_pos +=retVal;
        }

        fput(ftemp);

        return retVal;


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
        if(fd < 0 )
                return -EBADF;
        if(fd>=NFILES)
                return -EBADF;
        if(curproc->p_files[fd] == NULL)
                return -EBADF;

        file_t* ftemp =curproc->p_files[fd];
        curproc->p_files[fd]=NULL;
        fput(ftemp);
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
        
        if(fd < 0)
                return -EBADF;
        if(fd>=NFILES )
                return -EBADF;
        if(curproc->p_files[fd] == NULL)
                return -EBADF;

        int count=0,i=0;
        for(i=0;i<NFILES;i++)
        {
                if(curproc->p_files[fd]!=NULL)
                {
                        count++;
                }
        }
        if(count==NFILES-1)
                return -EMFILE;

        file_t* fOld = fget(fd);
        if(fOld == NULL)
        {
                return -EBADF;
        }

        int fdNew = get_empty_fd(curproc);
        /*KASSERT(-EMFILE != fdNew && " running out of file descriptors");*/
        if(fdNew == -EMFILE)
        {
                fput(fOld);
                return -EMFILE;
        }

        curproc->p_files[fdNew]=fOld;
        return fdNew;

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
        if(ofd < 0)
                return -EBADF;
        if(nfd < 0)
                 return -EBADF;
        if(ofd>= NFILES)
                return -EBADF;
        if(curproc->p_files[ofd]==NULL)
                return -EBADF;
        if(nfd >= NFILES)
                return -EBADF;


        file_t* fOld = fget(ofd);
        if(NULL == fOld)
            return -EBADF;

        if (ofd == nfd){
                fput(fOld);
                return nfd;
        }


        if(!( NULL == curproc -> p_files[nfd]))
            {
                do_close(nfd);
            }

        curproc -> p_files[nfd] = fOld;
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

        if( !(S_ISBLK(mode)) && !(S_ISCHR(mode)))
                return -EINVAL;


        size_t namelen;
        const char* name=NULL;
        vnode_t* base=NULL;
        vnode_t * res_node=NULL;


        int retVal = dir_namev(path, &namelen, &name, base, &res_node);
        if (retVal < 0)
        {
                return retVal;
        }

        if(  NAME_LEN-1 < namelen)
        {
            vput(res_node);
            return -ENAMETOOLONG;
        }

        vnode_t * tempNode;
        retVal = lookup(res_node, name, namelen, &tempNode);
        if(retVal == 0)
        {
            vput(res_node);
            vput(tempNode);
            return -EEXIST;
        }

        KASSERT(NULL != res_node->vn_ops->mknod && "function pointer not set ");
        dbg(DBG_ALL, "(GRADING2 3.b):res_node's mknod virtual function is valid and not NULL.\n");
        retVal= res_node->vn_ops->mknod(res_node, name, namelen, mode, devid);
        vput(res_node);

        return retVal;
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
        size_t length;
        const char *newDirname;
        vnode_t * base=NULL;
        vnode_t * resultnode;

        KASSERT(path != NULL && "invalid parameter sent");

        int returnVal = dir_namev(path, &length, &newDirname, base, &resultnode);

        if(returnVal < 0)
        {
                return returnVal;
        }

        if( NAME_LEN <= length  ){
                vput(resultnode);
                return -ENAMETOOLONG;
        }

        vnode_t * tempNode;
        returnVal = lookup(resultnode, newDirname, length, &tempNode);
        if(0 == returnVal){
                vput(resultnode);
                vput(tempNode);
                return -EEXIST;
        }

        KASSERT(NULL != resultnode->vn_ops->mkdir);
        dbg(DBG_ALL, "(GRADING2 3.c):res_node's mkdir virtual function is valid and not NULL.\n");
        returnVal = resultnode -> vn_ops -> mkdir(resultnode, newDirname, length);
        vput(resultnode);

        return returnVal;

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
      

        KASSERT(path != NULL && "invalid parameter sent");

   
        size_t length;
        const char *newDirname = NULL;
        vnode_t * base=  NULL;
        vnode_t * resultNode=NULL;

        int retVal = dir_namev(path, &length, &newDirname, base, &resultNode);

        if(0 != retVal)
                return retVal;

        if(  NAME_LEN <= length){
                vput(resultNode);
                return -ENAMETOOLONG;
        }

        if(  0 == strcmp(newDirname,".")){
                vput(resultNode);
                return -EINVAL;
        }

        if( 0 == strcmp(newDirname,"..")  ){
                vput(resultNode);
                return -ENOTEMPTY;
        }


        KASSERT(NULL != resultNode->vn_ops->rmdir && "function pointer not set");
        dbg(DBG_ALL, "(GRADING2 3.d):res_node's rmdir virtual function is valid and not NULL.\n");

        retVal = resultNode -> vn_ops -> rmdir(resultNode,newDirname,length);
        vput(resultNode);
        return retVal;
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

        KASSERT(NULL != path &&  "invalid parameter" );

        vnode_t *res_node = NULL;
        size_t length = 0;
        const char *newFilename = NULL;
        vnode_t * baseN =NULL;
        vnode_t *res_node2 =NULL;

        int retVAl = dir_namev(path, &length, &newFilename, baseN, &res_node);
        if(retVAl > 0 || retVAl <0)
        {
                return retVAl;
        }

        if(NAME_LEN-1 < length )
        {
                vput(res_node);
                return -ENAMETOOLONG;
        }

        retVAl = lookup(res_node, newFilename, length, &res_node2);
        if(  retVAl > 0  || retVAl < 0 )
        {
                vput(res_node);
                return retVAl;
        }

        if(S_ISDIR(res_node2->vn_mode))
        {
                vput(res_node);
                vput(res_node2);
                return -EISDIR;

        }

        KASSERT(NULL != res_node->vn_ops->unlink);
        dbg(DBG_ALL, "(GRADING2 3.e):res_node's unlink virtual function is valid and not NULL.\n");

        retVAl = res_node->vn_ops->unlink(res_node, newFilename, length);

        vput(res_node);
        vput(res_node2);
        return retVAl;
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
    int retVal = 0;
    vnode_t* res_vnode = NULL;
  
    
  if((MAXPATHLEN < strlen(to) ) || (  MAXPATHLEN < strlen(from)))
    return -ENAMETOOLONG;
  
  
    const char* naav;
    size_t length = 0;
    vnode_t* tempVnode = NULL;
    
   retVal = open_namev( from, O_RDWR, &res_vnode, NULL);
  if(retVal<0)
    return retVal;

  if(S_ISDIR(res_vnode->vn_mode))
    {
      vput(res_vnode);
      return -EISDIR;
    }
  
    
   
  retVal = dir_namev(to, &length, &naav, NULL, &tempVnode);
  vnode_t* tempVnode1= NULL;
  if(0 > retVal)
    {
      vput(res_vnode);
      return retVal;
    }
   
  retVal = lookup(tempVnode, naav, length, &tempVnode1);
  if(0 <= retVal)
    {
          vput(tempVnode1);
      vput(res_vnode);
      
      return -EEXIST;
    }
  
  retVal = tempVnode->vn_ops->link(res_vnode, tempVnode, naav, length);
  
  vput(res_vnode);
  vput(tempVnode);
  
  return retVal;
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


        KASSERT(oldname != NULL  && newname != NULL && "invalid parameters");

        int Val = 0;
      

        Val = do_link(oldname, newname);

        if(0 > Val || 0 < Val)
        {

            return Val;
        }
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
        

        KASSERT(NULL != path && "Invalid parameter");


        vnode_t * newDirNode=NULL;

        vnode_t* base=NULL;
        const char* parentDirName = NULL;
       
        vnode_t * node_out;

        size_t DirNamelength;
        int Val;

        if ((Val = dir_namev(path, &DirNamelength, &parentDirName, base, &node_out)) < 0)
        {
                return Val;
        }

        if(NAME_LEN <= DirNamelength )
        {
            vput(node_out);
            return -ENAMETOOLONG;
        }

        Val = lookup(node_out, parentDirName, DirNamelength,&newDirNode);

        if(0 < Val || 0 > Val )
        {
                vput(node_out);
            return Val;
        }

        if(!S_ISDIR(newDirNode -> vn_mode))
        {
                vput(node_out);
            vput(newDirNode);
            return -ENOTDIR;
        }
        vput(node_out);
        vput(curproc ->p_cwd);
        curproc ->p_cwd = newDirNode;
        return 0;

}

/* Call the readdir f_op on the given fd, filling in the given dirent_t*.
 * If the readdir f_op is successful, it will return a positive value which
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

        KASSERT(dirp != NULL && "invalid parameter");
        int maxFiles = NFILES;
        int val = 0;
        file_t *ptr2File = NULL;


        if( maxFiles <= fd)
                return -EBADF;

        if( 0 > fd)
                return -EBADF;

        ptr2File= fget(fd);
        if( NULL == ptr2File)
        
                return -EBADF;
        

        if(!S_ISDIR(ptr2File->f_vnode->vn_mode))
        {

                fput(ptr2File);
                return -ENOTDIR;

        }
        KASSERT (ptr2File->f_vnode->vn_ops->readdir != NULL && "function pointer not set");
        val = ptr2File->f_vnode->vn_ops->readdir(ptr2File->f_vnode, ptr2File->f_pos, dirp);

        if(0 < val)
        {
                ptr2File->f_pos = ptr2File->f_pos+ val;
                fput(ptr2File);

                return (sizeof(dirent_t));

        }
        else {
            fput(ptr2File);
            return val;
        }
        return val;
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



    if(!(  2 == whence  || 0 == whence || 1== whence ))
        {
            return -EINVAL;
        }

    if(0 > fd || NFILES <= fd )
        {
            return -EBADF;
        }



    if(NULL == curproc->p_files[fd]){
            return -EBADF;
    }



    int pos = curproc->p_files[fd]->f_pos;


    if(whence == SEEK_CUR)
        {
            pos = pos + offset;
        }
    else if(whence == SEEK_SET)
        {
            pos = offset;
        }
    else if (whence == SEEK_END)
        {
            pos = offset + curproc->p_files[fd]->f_vnode->vn_len ;
        }


    if(0 > pos )
        {

            return -EINVAL;
        }
    curproc->p_files[fd]->f_pos = pos;


    return pos;
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
  
  
  
  if(MAXPATHLEN < strlen(path) )
    return -ENAMETOOLONG;

  if(NULL == buf)
    return -EFAULT;
  
  vnode_t* tempObj = NULL; 
  int val = open_namev(path,O_RDONLY,&tempObj,NULL);
  if(0 > val)
    return val;
  
  if(NULL != tempObj->vn_ops->stat)
  val = tempObj->vn_ops->stat(tempObj, buf);

  vput(tempObj);
  
  return val;           
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
