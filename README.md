CPSC 457 Assignment 3 - README Johnny Phuong Chung

DESCRIPTION:
- The purpose of this program is to use thread synchronization tools to construct a coffee shop simulation with "Train" and ""Space" models.

COMPILE/RUN:
- navigate to the directory containing the main a3.c file
- use command $make to compile and link program via Makefile
- use command $./a3 with command line arguments, as defined in USAGE

USAGE: $./a3 [-bar_model] [bar_num] [cust_num]
-   bar_model = option for barista model to run for simulation
-   bar_model: 't' to run Train Model
-   bar_model: 's' to run Space Model
- bar_num = number of baristas for the model
- cust_num = number of customers for simulation

ASSUMPTIONS/COMMENTS:
- space model generates only one cashier barista, all other baristas delegated to clearing orders
- MAX_BUFFER_SIZE should never be exceeded, and as such 2 symmetrical semaphores are used to control each buffer
- the use of the sleep() and usleep() functions are to reduce the undesired effect of busywaiting
- Simulations 1 and 2 are run using the standard a3.c file
- Simulations 3 and 4 are run using respectively modified versions of the a3.c file
- Disk Scheduling totals calculated as demonstrated by Tyson Kendon
- No error checking for more than 4 command line arguments, focus on model and number of baristas/customers
- erroneous returns from skeleton code can be removed/modified and will not affect simulations

SOURCES:
- http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread.h.html
- http://man7.org/linux/man-pages/man3/usleep.3.html
- http://stackoverflow.com/questions/6179419/elapsed-time-in-c
- http://man7.org/linux/man-pages/man3/sem_timedwait.3.html
- http://man7.org/linux/man-pages/man3/sem_post.3.html
- http://man7.org/linux/man-pages/man3/pthread_cancel.3.html
- https://pages.cpsc.ucalgary.ca/~mrzakeri/457_s16/Worksheet2-MultithreadedProgramming.txt
- Chris Kinzel (assisted in bash scripts for simulation evaluation)