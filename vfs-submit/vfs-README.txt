Documentation for Kernel Assignment 2
=====================================

+-------+
| BUILD |
+-------+

Comments: Use "make" command

+---------+
| GRADING |
+---------+

(A.1) In fs/vnode.c:
    (a) In special_file_read(): 6 out of 6 pts
    (b) In special_file_write(): 6 out of 6 pts

(A.2) In fs/namev.c:
    (a) In lookup(): 6 out of 6 pts
    (b) In dir_namev(): 10 out of 10 pts
    (c) In open_namev(): 2 out of 2 pts

(3) In fs/vfs_syscall.c:
    (a) In do_write(): 6 out of 6 pts
    (b) In do_mknod(): 2 out of 2 pts
    (c) In do_mkdir(): 2 out of 2 pts
    (d) In do_rmdir(): 2 out of 2 pts
    (e) In do_unlink(): 2 out of 2 pts
    (f) In do_stat(): 2 out of 2 pts

(B) vfstest: 40 out of 40 pts
    Comments: works fine
	
/* Given test case - vfstest */
   -------------------------

Test Case: This test case is testing the given vfstest --> vfstest_main()

KSHELL command: vfstest

Expected Output: Our code executes successfully with the following status:

		tests completed:
				506 passed
				0 failed


(C) Self-checks :

KSHELL COMMANDS		: 	SHORT DESCRIPTION
---------------		 	-----------------
readTest		:	Read operation tests edge case on filedescriptor fd
writeTest		:	Write operation tests edge case on filedescriptor fd
renameInvalidTest	:	Rename non-existent file
renameDirTest		:	Rename directory


Detailed Description of additional self checks :
------------------------------------------------

/* Self-check - readTest */
   -----------------------------

Test Case: 	This test case has been inserted to check for the edge/boundary conditions of do_read() function. Herein, we are checking how exactly the function behaves if the entered fd is greater than the maximum permissible value (NFILES). The maximum value is 32 and any input above this value should result in the code terminating from that point.

KSHELL command: readTest

Expected Output: Since the entered fd has a value of 33 (greater than the maximum permissible value - NFILES), the do_read() function terminates 			saying that fd is out of limit.

		main/kmain.c:524 checkRead(): 
		STUDENT TEST: Read operation was not successful - fd (33) is out of limit


/* Self-check- writeTest */
   ------------------------------

Test Case: 	This test case has been inserted to check for the edge/boundary conditions of do_write() function. Herein, we are checking how 	exactly the function behaves if the entered fd is greater than the maximum permissible value (NFILES). The maximum value is 32 and 			any input above this value should result in the code terminating from that point.

KSHELL command: writeTest

Expected Output: Since the entered fd has a value of 33 (greater than the maximum permissible value - NFILES), the do_write() function terminates 			saying that fd is out of limit.

		main/kmain.c:536 checkWrite(): 
		STUDENT TEST: Write operation was not successful - fd (33) is out of limit


/* Self-check - renameInvalidTest */
   -------------------------------

Test Case: 	This test case has been inserted to check for the edge/boundary condition of do_rename() function. Herein, we are checking the 	functionality of the do_rename() function if it is fed with an old filename that does not exist and a new filename which we want the old filename to be renamed to. If the old filename does not exist, it will throw an error saying that the rename operation failed 			because old filename does not existand will terminate.

KSHELL command: renameInvalidTest

Expected Output: Since the entered old filename "/test/file2" exists, the do_rename() function is unable to execute successfully and hence throws an 			error saying that the do_rename() failed because the file does not exist.

		STUDENT TEST: Rename operation executing
		main/kmain.c:586 checkRename2(): 
		STUDENT TEST: Rename operation failed because the old filename does not exist

/* Self-check - renameDirTest */
   -----------------------------

Test Case: 	This test case has been inserted to check for the edge/boundary condition of do_rename() function. Herein, we are checking the 	functionality of the do_rename() function if it is fed with an old filename that is a directory and a new filename which we want the old filename to be renamed to. If the old filename is a directory, this function will throw an error saying that the rename operation 			failed because old filename is a directory and will terminate.

KSHELL command: renameDirTest

Expected Output: Since the entered old filename "/test" is a directory, the do_rename() function is unable to execute successfully and hence throws an error saying that the do_rename() failed because old filename is a directory. Since unlink called within rename works on files and not directories, it fails and hence creates another link to the existing directory.

		STUDENT TEST: Rename operation executing
		main/kmain.c:609 checkRename3(): 
		STUDENT TEST: Rename operation failed because the old filename is a directory

		
-------------------------------------------------------------------------------------------------------------
/* End of Self checks */
-------------------------------------------------------------------------------------------------------------


    Comments: (please provide details)

Missing required section(s) in README file (vfs-README.txt): No, all sections complete
Submitted binary file : No, we haven't submitted any binary file
Submitted extra (unmodified) file : No, we haven't submitted any extra file
Wrong file location in submission : No
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : No, we have used all the dbg(DBG_PRINT, ...) statements for printing
Not properly identify which dbg() printout is for which item in the grading guidelines : No, the dbg's can be properly identified and they are written corresponding to the KASSERT
Cannot compile : No, can be compiled properly
Compiler warnings : No compiler warnings
"make clean" : Works as required
Useless KASSERT : No, all the KASSERT's are useful
Insufficient/Confusing dbg : No, all the dbg's are to the point
Kernel panic : No, there is no kernel panic
Cannot halt kernel cleanly : No, can halt the kernel cleanly

+------+
| BUGS |
+------+

Comments:  No bugs

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

If not equal-share contribution, please list percentages.

+-------+
| OTHER |
+-------+

Comments on design decisions: No comments
Comments on deviation from spec: There is no deviation from the specs
