Documentation for Kernel Assignment 1
=====================================

+-------+
| BUILD |
+-------+

Comments: Use "make" command

+---------+
| GRADING |
+---------+

(A.1) In main/kmain.c:
    (a) In bootstrap(): 3 out of 3 pts
    (b) In initproc_create(): 3 out of 3 pts

(A.2) In proc/proc.c:
    (a) In proc_create(): 4 out of 4 pts
    (b) In proc_cleanup(): 5 out of 5 pts
    (c) In do_waitpid(): 8 out of 8 pts

(A.3) In proc/kthread.c:
    (a) In kthread_create(): 2 out of 2 pts
    (b) In kthread_cancel(): 1 out of 1 pt
    (c) In kthread_exit(): 3 out of 3 pts

(A.4) In proc/sched.c:
    (a) In sched_wakeup_on(): 1 out of 1 pt
    (b) In sched_make_runnable(): 1 out of 1 pt

(A.5) In proc/kmutex.c:
    (a) In kmutex_lock(): 1 out of 1 pt
    (b) In kmutex_lock_cancellable(): 1 out of 1 pt
    (c) In kmutex_unlock(): 2 out of 2 pts

(B) Kshell : 20 out of 20 pts
    Comments:  The DRIVERS = 1 is set in Config.mk file and the help, echo,exit commands work properly 

(C.1) waitpid any test, etc. (4 out of 4 pts)
(C.2) Context switch test (1 out of 1 pt)
(C.3) wake me test, etc. (2 out of 2 pts)
(C.4) wake me uncancellable test, etc. (2 out of 2 pts)
(C.5) cancel me test, etc. (4 out of 4 pts)
(C.6) reparenting test, etc. (2 out of 2 pts)
(C.7) show race test, etc. (3 out of 3 pts)
(C.8) proc kill test (2 out of 2 pts)

(D.1) sunghan_test(): producer/consumer test (10 out of 10 pts)
(D.2) sunghan_deadlock_test(): deadlock test (5 out of 5 pts)

(E) Additional self-checks :
    Comments: 
	
KSHELL COMMANDS		: 	SHORT DESCRIPTION
---------------		 	-----------------
invalid_pid		:	Checks invalid pid, Should fail KASSERT
idle_pid		:	Runs edge condition of waitpid for IDLE proc
mutex_lock		:	Try to lock same mutex twice, it should fail locking
mutex_unlock		:	Process fails while trying to unlock a mutex which is locked by another thread
mutex_deadlock		:	Processes get stuck in deadlock trying to acquire mutex held by each other
reverse_kill		:	Kill all the processes in reverse order


Detailed Description of additional self checks :
------------------------------------------------



/* Self-check - invalid_pid */
   ------------------------------------

Test Case: This test case has been inserted to check for the edge/boundary conditions of do_waitpid() function. Herein, we are checking how exactly the function behaves for an invalid input, i.e., for "pid" values less than or equal to -2.

KSHELL command: invalid_pid

Expected Output: Since the entered pid is invalid (negative value less than -2), the function exits at the following KASSERT statement:
	KASSERT((pid == -1 || pid > 0) && "pid can only be -1 or a positive value");

	panic in proc/proc.c:383 do_waitpid(): assertion failed: (pid == -1 || pid > 0) && "pid can only be -1 or a positive value"
	Kernel Halting.


/* Self-check - idle_pid */
   ---------------------------------

Test Case: This test case has been inserted to check for the edge/boundary conditions of do_waitpid() function. Herein, we are checking how exactly the function behaves if the entered pid is '0', i.e., pid for IDLE process.

KSHELL command: idle_pid

Expected Output: Since the entered pid belongs to IDLE process (pid = 0), the function exits at the following KASSERT statement:
	KASSERT((pid == -1 || pid > 0) && "pid can only be -1 or a positive value");

	panic in proc/proc.c:383 do_waitpid(): assertion failed: (pid == -1 || pid > 0) && "pid can only be -1 or a positive value"
	Kernel Halting.



/* Self-check - mutex_lock */
   ------------------------------------

Test Case: This test case simulates the condition where a thread in the process tries to lock the same mutex twice. 

KSHELL command: mutex_lock

Expected Output: Since, this is an invalid behaviour, KASSERT in kmutex_lock is seen.

	panic in proc/kmutex.c:37 kmutex_lock(): assertion failed: (curthr && (mtx->km_holder !=curthr)) && "kmutex_lock(): Thread already holds the mutex"
	Kernel Halting.



/* Self-check - mutex_unlock */
   ------------------------------------

Test Case: This test case simulates the case of one thread trying to unlock a mutex locked by some other thread. 

KSHELL command: mutex_unlock

Expected Output: Mutex should always be locked/unlocked by the same thread, hence this will result in KASSERT.

	panic in proc/kmutex.c:95 kmutex_unlock(): assertion failed: (curthr && (mtx->km_holder ==curthr)) && "kmutex_unlock(): Current thread doesn't hold a mutex to unlock"
	Kernel Halting.

				
/* Self-check - mutex_deadlock */
   ------------------------------------

Test Case: This test case simulates the case of two processes getting into deadlock when they try to wait for the mutex held by each other.
Process 1 acquires mutex 1
Process 2 acquires mutex 2

Process 1 now tries to acquire mutex 2, which is held by Process 2
Process 2 now tries to acquire mutex 1, which is held by Process 1


KSHELL command: mutex_deadlock

Expected Output: Both processes, Process 1 and Process 2 will be blocked on wait queue for mutexes acquired by each other resulting in Deadlock.


		
/* Self-check - reverse_kill */
   ------------------------------------

Test Case: This test case simulates the case of creating processess and then killing each process in the reverse order.

A process is created by the Init process.
This process creates 5 child processes.
Then, all these 5 chid processes is killed in reversed order.

KSHELL command: reverse_kill

Expected Output: All the 5 child processes are expected to exit cleanly. 
		Post that, kshell prompt returns to the user and when the user enters "exit" in the QEMU terminal, the promt is shown "It is safe to turn off your computer."



-------------------------------------------------------------------------------------------------------------
/* End of Self checks */
-------------------------------------------------------------------------------------------------------------


Missing required section(s) in README file (procs-README.txt): No, all sections complete
Submitted binary file : No, we haven't submitted any binary file
Submitted extra (unmodified) file : No, we haven't submitted any extra file
Wrong file location in submission : No
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : No, we have used all the dbg(DBG_PRINT, ...) statements for printing
Not properly identify which dbg() printout is for which item in the grading guidelines : No, the dbg's can be properly identified and they are written corresponding to the KASSERT
Cannot compile : No, can be compiled properly
Compiler warnings : No compiler warnings
"make clean" : Works as required
Useless KASSERT : No, all the KASSERT's are useful
Insufficient/Confusing dbg :  No, all the dbg's are to the point
Kernel panic : No, there is no kernel panic
Cannot halt kernel cleanly : No, can halt the kernel cleanly

+------+
| BUGS |
+------+

Comments: No bugs

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

All the team members have contributed equally towards the completion of Kernel assignment 1.


If not equal-share contribution, please list percentages.

+-------+
| OTHER |
+-------+

Comments on design decisions: As per specifications
Comments on deviation from spec: No deviation

