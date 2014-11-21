Group members:
	pkchaudh@usc.edu
	gote@usc.edu
	akshayrd@usc.edu
	neelamag@usc.edu

Every team member has equal contribution.

All the following test cases can be run with DYNAMIC=1 or DYNAMIC=0.

After launching weenix via command "./weenix -n", weenix prompt is launched and following userland program can be executed. 

segfault  OR  /usr/bin/segfault
hello     OR  /usr/bin/hello
args  ab cde fghi j   OR /usr/bin/args  ab cde fghi j
/bin/uname  OR uname
kshell    OR /usr/bin/kshell
ls        OR /bin/ls
/sbin/halt
/bin/sh
/usr/bin/spin
/usr/bin/forkbomb - It works for 3.5 min till 350 process creation then it panics
/usr/bin/stress - Same as above after successful initial tests.
check
vfstest  OR  /usr/bin/vfstest - All tests 578 are passed 
/usr/bin/memtest - panics after sometime
/usr/bin/eatmem -  
/bin/ed

--------- SELF TESTS ---------

Run kshell command on weenix prompt before performing EACH of the following test cases. It will launch kshell$ prompt and all the following commands available in the kshell menu can now be executed. (type help) 

KNOWN PROBLEMS:
        - Sometimes we observed that, Running the command on the Kshell prompt doesnot do anything. 
          It is a very random behaviour. If this is encountered, please try re-running the command. 


testhello - Launches the hello world userland program
testsh    - Launches the shell userland program, Expected : weenix prompt will open. Type exit once to exit from this prompt.
testspin  - Launches the spin userland program(infinite loop)
testls    - Launches the ls userland program
testhalt  - Launches the halt userland program to halt system
testEd    - Launches the Editor userland program - Try performing basic editor operations.
