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
        /* NOT_YET_IMPLEMENTED("VFS: lookup"); */

	/* KASSERT statements as per grading guidelines */
	KASSERT(NULL != dir);
	dbg(DBG_PRINT,"\n(GRADING2A 2.a) dir = %ld\n",(long)dir->vn_vno);

        KASSERT(NULL != name);
	dbg(DBG_PRINT,"\n(GRADING2A 2.a) name = %s\n",name);

        KASSERT(NULL != result);
	dbg(DBG_PRINT,"\n(GRADING2A 2.a) result not null\n");

	/* If dir has no lookup(), return -ENOTDIR */
	if (!S_ISDIR(dir->vn_mode) || (dir->vn_ops->lookup == NULL))
	{
		dbg(DBG_ERROR,"\nlookup(): dir is not a directory or has no lookup\n");
		return -ENOTDIR;
	}

	if(len > NAME_LEN)
	{
		dbg(DBG_ERROR, "\n A filename too long.");
		return -ENAMETOOLONG;
	}

	if(len == 0)
	{
	   *result = vget(dir->vn_fs, dir->vn_vno);
           dbg(DBG_ERROR, "\nlookup(): len is 0.\n");
	   return 0;
	}

	return dir->vn_ops->lookup(dir,name,len,result);

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
	KASSERT(NULL != pathname);
	dbg(DBG_PRINT,"\n(GRADING2A 2.b) pathname = %s\n",pathname);
        KASSERT(NULL != namelen);
	dbg(DBG_PRINT,"\n(GRADING2A 2.b) namelen not NULL\n");
        KASSERT(NULL != name);
	dbg(DBG_PRINT,"\n(GRADING2A 2.b) name not NULL\n");
        KASSERT(NULL != res_vnode);
	dbg(DBG_PRINT,"\n(GRADING2A 2.b) res_vnode not NULL\n");

	if(strlen(pathname) > MAXPATHLEN)
	{
	    dbg(DBG_ERROR, "\ndir_namev(): PathName is too long\n");
	    return -ENAMETOOLONG;
	}

	if(strlen(pathname) <= 0)
	{
	    dbg(DBG_ERROR, "\ndir_namev(): PathName is too short\n");
	    return -EINVAL;
	}


	int errno = 0, lookup_done = 0,len = 0;

	/* Set NULL base to crurent pwd */
	if ( base == NULL )
	{
		base = curproc->p_cwd;
	}

	/* if pathname starts with /, use root*/
	if (pathname[0] == '/')
		base = vfs_root_vn;

	int startP = 0;
	if (pathname[0] == '/') startP++;

	/*Find file (maybe a dir) in current dir*/
	while ( 1 )
	{
		/*Loop through pathname, getting name and len*/
		for ( len = 0; pathname[startP+len] != '/'; len++)
		{
			if (pathname[startP+len] == '\0')
			{
				if ( len > NAME_LEN)
				{
					dbg(DBG_ERROR, "\n A filename too long.");
					return -ENAMETOOLONG;
				}
				KASSERT(NULL != namelen);
				KASSERT(NULL != name);

				/*last vnode found is not a dir*/
				*res_vnode = vget(base->vn_fs,base->vn_vno);
				KASSERT(NULL != *res_vnode);
				dbg(DBG_PRINT,"\n(GRADING2A 2.b) pointer to corresponding vnode is not NULL\n");
				if (!S_ISDIR((*res_vnode)->vn_mode))
				{
					vput(*res_vnode);
					dbg(DBG_ERROR,"\n vnode is not a directory\n");
					return -ENOTDIR;
				}

				*name = &pathname[startP];
				*namelen = len;	
				return 0;
			}
		}
		/*Do lookup on name IFF its not basename*/
		errno = lookup(base,&pathname[startP],len,res_vnode);
		if (errno < 0)
		{
			return errno;
		}

		/*Lookup returned ok, so keep going*/
		vput(*res_vnode);
		base = *res_vnode;
		startP = startP + len + 1;
	}

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
        /* NOT_YET_IMPLEMENTED("VFS: open_namev"); */
	
	KASSERT(NULL != pathname);
	dbg(DBG_PRINT,"\nopen_namev: pathname = %s\n",pathname);
        KASSERT(NULL != res_vnode);
	dbg(DBG_PRINT,"\nopen_namev: res_vnode not NULL\n");
	
	if(strlen(pathname) > MAXPATHLEN)
	{
	    dbg(DBG_ERROR, "\nopen_namev(): PathName is too long\n");
	    return -ENAMETOOLONG;
	}

	if(strlen(pathname) <= 0)
	{
	    dbg(DBG_ERROR, "\nopen_namev(): PathName is too short\n");
	    return -EINVAL;
	}

	const char *name = NULL;
	size_t len = 0;

        int retVal = dir_namev(pathname,&len,&name,base,res_vnode);
	if ( retVal < 0 )
	{
		dbg(DBG_ERROR, "\nopen_namev():  dir_namev failed to find the specified vnode\n");
	        return retVal;
	}
	vput(*res_vnode);

	/* Lookup name and create if fails */
	retVal = lookup(*res_vnode,name,len,res_vnode);
	if ( retVal == -ENOENT && (flag & O_CREAT) == O_CREAT)
	{
		dbg(DBG_ERROR,"\nopen_namev: lookup failed, name = %s, creating\n",name);
		KASSERT(NULL != (*res_vnode)->vn_ops->create);
		dbg(DBG_PRINT,"\n(GRADING2A 2.c) (*res_vnode)->vn_ops->create is not NULL\n");
		retVal = (*res_vnode)->vn_ops->create(*res_vnode,name,len,res_vnode);	
	}

	return retVal;	
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
