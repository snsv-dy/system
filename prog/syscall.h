#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYSCALL_PUTS 0
#define SYSCALL_GETS 1
#define SYSCALL_SWITCH_TASK 2
#define SYSCALL_SLEEP 3
#define SYSCALL_CREATE_THREAD 4
#define SYSCALL_EXIT 5
#define SYSCALL_SEM_WAIT 6
#define SYSCALL_SEM_POST 7
#define SYSCALL_START_PROCESS 8
#define SYSCALL_WAIT_FOR_FINISH 9
#define SYSCALL_SEM_OPEN 10
#define SYSCALL_SEM_UNLINK 11
#define SYSCALL_SEM_RAND 12

extern int syscall(unsigned int call_type, void *data);

#endif