/*

	Global Types used by other source files
	
	Author: Boris Kim

*/

// Global definitions for objects created by RTOS

#ifndef __GLOBAL_TYPES_H
#define __GLOBAL_TYPES_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "uart.h"

#ifndef __RTGT_UART
#define __RTGT_UART
#endif

#define STIME 1000
#define NUM_PRIORITIES 4
#define NUM_TCB 6
#define IDLE_ID 77

//#define __DEBUG

typedef enum {
	osNoError 				=  0,
	osError   				= -1,
	osErrorOverflow 		= -2,
	osErrorPerm 			= -3,
	osErrorInv 				= -4,
	osErrorEmp				= -5
} osError_t;

// Priority Enum
typedef enum {
	osPriorityNone	= 0,
	osPriorityLow   = 1,
	osPriorityMed		= 2,
	osPriorityHigh 	= 3
} priority_t;

// Task State 
typedef enum {
	T_INACTIVE 	= 0,
	T_READY 		= 1,
	T_RUNNING 	= 2,
	T_BLOCKED 	= 3
} taskState_t;

typedef uint32_t tid_t; 

typedef struct {
	tid_t tid;
	uint32_t *stackPointer;
	uint32_t *stackBaseAddress;
	uint32_t *stackOverflowAddress;
	void *nextTcb;
	taskState_t state;
	priority_t priority;
} tcb_t;
	
typedef struct {
	uint32_t size;
	tcb_t *head;
	tcb_t *tail;
} tcbList_t;

typedef struct {
	uint32_t count;
	tcbList_t blockedList;
} sem_t;

typedef struct {
	bool available;
	bool inherited;
	tcb_t *owner;
	priority_t originalPriority;
	tcb_t *blockedTask;
} mutex_t;

typedef void (*osThreadFunc_t) (void *argument);

typedef struct {
	tcb_t *currTCB;
	tcbList_t readyQueueList[NUM_PRIORITIES];
	priority_t currPriority;
} scheduler_t;

#endif //__GLOBAL_TYPES_H
