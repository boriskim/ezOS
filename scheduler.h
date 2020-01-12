/*

	Header file for scheduler functions
	
	Author: Boris Kim

*/

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <LPC17xx.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "context.h"
#include "global_types.h"

/**********************************************TCB METHODS*************************************************/

// Content print methods 
void printGlobalLocations(void);
void printTcbContents(tcb_t *tcb);
void printListContents(tcbList_t *list);
void printSchedulerStatus(void);

// TCB stack methods
osError_t tcb_push(tcb_t *tcb, uint32_t content);

// TCB list queueing methods
osError_t tcbList_enqueue(tcbList_t *list, tcb_t* tcb);
tcb_t *tcbList_dequeue(tcbList_t *list);

// TCB state changing method. Detects if scheduler needs to be run
osError_t changeState(tcb_t *tcb, taskState_t newState);
/**********************************************TCB METHODS*************************************************/

// Context switch methods
osError_t contextSwitch(tcb_t *oldTCB, tcb_t *newTCB);
tcbList_t *findNextTask(void);

// ISR's
void PendSV_Handler(void);
void SysTick_Handler(void);

// Scheduler initialize method. Sets ready queues to blank lists
void initScheduler(void);

#endif //__SCHEDULER_H
