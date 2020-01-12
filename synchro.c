/*

	Source file for synchronization methods
	
	Author: Boris Kim

*/

#include "synchro.h"

/**********************************************GLOBAL VARIABLES********************************************/
// Boolean to determine early pre-empting
extern bool runScheduler;

// Global scheduler
extern scheduler_t scheduler;

// TCB's
extern tcb_t tcb[NUM_TCB];
extern tcb_t main_tcb;
/**********************************************GLOBAL VARIABLES********************************************/

void osSemaphoreInit(sem_t *sem, uint32_t count) {
	sem->count = count;
	
	tcbList_t blankList;
	blankList.size = 0;
	blankList.head = NULL;
	blankList.tail = NULL;
	
	sem->blockedList = blankList;
}

osError_t osSemaphoreLend(sem_t *sem) {
	__disable_irq();
	printf("Semaphore lend enter, Semaphore count: %d\n", sem->count);
	if(sem->count <= 0) {
		// Change state of current task to blocked
		changeState(scheduler.currTCB, T_BLOCKED);
		tcbList_enqueue(&sem->blockedList, scheduler.currTCB);
		tcbList_dequeue(&scheduler.readyQueueList[scheduler.currPriority]);
		
		printf("Was blocked. Semaphore count: %d\n", sem->count);
		while(sem->count <= 0) {
			__enable_irq();
			while(runScheduler == true) {
				printf("");
			}
			__disable_irq();
		}
	}
	
	(sem->count)--;
	printf("Semaphore lend exit, Semaphore count: %d\n", sem->count);
	__enable_irq();
	return osNoError;
}

osError_t osSemaphoreReturn(sem_t *sem) {
	__disable_irq();
	printf("Semaphore return enter, Semaphore count: %d\n", sem->count);
	(sem->count)++;
	if(sem->blockedList.size != 0) {
		tcb_t *unblockedTask = tcbList_dequeue(&sem->blockedList);
		changeState(unblockedTask, T_READY);
		tcbList_enqueue(&scheduler.readyQueueList[unblockedTask->priority], unblockedTask);
	}
	printf("Semaphore return exit, Semaphore count: %d\n", sem->count);
	__enable_irq();
	return osNoError;
}


void osMutexInit(mutex_t *mut) {
	mut->available = true;
	mut->inherited = false;
	mut->originalPriority = osPriorityNone;
	mut->owner = NULL;
	mut->blockedTask = NULL;
}

osError_t osMutexLock(mutex_t *mut) {
	__disable_irq();
	
	// Check if mutex is already acquired
	if(mut->available == false) {
		
		if(mut->owner->tid == scheduler.currTCB->tid) {
			printf("Cannot acquire mutex twice\n");
			__enable_irq();
			return osErrorInv;
		}
		// Place blocked task in mutex blocked task
		mut->blockedTask = scheduler.currTCB;
		changeState(mut->blockedTask, T_BLOCKED);
		
		// Dequeue the blocked task
		tcbList_dequeue(&scheduler.readyQueueList[scheduler.currPriority]);
		
		// Check if priority inheritance is necessary
		if(scheduler.currPriority > mut->originalPriority) {
			mut->inherited = true;
			mut->owner->priority = scheduler.currPriority;
			
			// Enqueue instance of inherited prioirty task
			tcbList_enqueue(&scheduler.readyQueueList[mut->blockedTask->priority], mut->owner);
		}
		__enable_irq();
		while(runScheduler == true) {
			printf("");
		}
		__disable_irq();
	}
	
	// Mutex is available
	mut->available = false;
	mut->originalPriority = scheduler.currPriority;

	mut->owner = scheduler.currTCB;
	
	__enable_irq();
	return osNoError;
}

osError_t osMutexUnlock(mutex_t *mut) {
	__disable_irq();
	
	if(mut->available == true) {
		printf("WARNING: available mutex cannot be unlocked\n");
		__enable_irq();
		return osErrorInv;
	}
	
	if(scheduler.currTCB != mut->owner) {
		printf("WARNING: mutex not owned by this task, cannot be unlocked by it\n");
		__enable_irq();
		return osErrorPerm;
	}
	
	// Check if there is a blocked task
	if(mut->blockedTask != NULL) {
		// Change the state of blocked tasked from blocked to ready
		changeState(mut->blockedTask, T_READY);
		// Enqueue task to the scheduler
		tcbList_enqueue(&scheduler.readyQueueList[mut->blockedTask->priority], mut->blockedTask);
		mut->blockedTask = NULL;
	}
	// Check if priority was inherited
	if(mut->inherited == true) {
		// Remove inherited task from the inherited prioirty queue
		tcbList_dequeue(&scheduler.readyQueueList[mut->owner->priority]);
		
		mut->owner->priority = mut->originalPriority;
		mut->available = true;
		mut->inherited = false;
		mut->owner = NULL;
		mut->originalPriority = osPriorityNone;
		
		changeState(scheduler.currTCB, T_READY);
		runScheduler = true;
		__enable_irq();
		while(runScheduler == true) {
			printf("");
		}
		__disable_irq();
	}
	else {
		mut->available = true;
		mut->owner = NULL;
		mut->originalPriority = osPriorityNone;
	}
	__enable_irq();
	return osNoError;
}
