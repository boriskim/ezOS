/*

	Source file for scheduler functions
	
	Author: Boris Kim

*/

#include "scheduler.h"

/***************************************GLOBAL DECLARATIONS****************************************************/
const uint32_t timeSlice = STIME;
uint32_t msTicks = 0; // counter for timeslice
uint32_t countDown = timeSlice;

// Global scheduler boolean
bool runScheduler = false;

// Global TCB
tcb_t tcb[NUM_TCB];
tcb_t main_tcb;

// Global scheduler
scheduler_t scheduler;
	
static const uint32_t SET_PENDSV = 1 << 28;
static const uint32_t CLEAR_PENDSV = 1 << 27;
/***************************************GLOBAL DECLARATIONS****************************************************/

void printGlobalLocations(void) {
	//printf("runScheduler: %p, value is %d, scheduler: %p, TCB Array: %p, main TCB: %p\n", &runScheduler, (uint32_t)runScheduler, &scheduler, &tcb, &main_tcb);
}

void printTcbContents(tcb_t *tcb) {
	printf("\nTID: %d, P: %d, State: %d bsp: %p, sp: %p\n", tcb->tid, tcb->priority, tcb->state, tcb->stackBaseAddress, tcb->stackPointer);
}

void printListContents(tcbList_t *list) {
	printf("List %d Status: head: %d, tail: %d", list->head->priority, list->head->tid, list->tail->tid);
}

osError_t tcb_push(tcb_t *tcb, uint32_t content) {
	(tcb->stackPointer)--;
	
	if(tcb->stackPointer <= tcb->stackOverflowAddress) {
		// Return error message if overflow
		printf("ERROR: STACK OVERFLOW. TASK ID: %d/n", tcb->tid);
		return osErrorOverflow;
	}
	
	*(tcb->stackPointer) = content;
	return osNoError;
}

osError_t tcbList_enqueue(tcbList_t *list, tcb_t *tcb) {
	#ifdef __DEBUG
	printf("\ntcbList_enqueue: Enter, enqueue TID: %d\n", tcb->tid);
	#endif
	
	// Check if the list is empty. If yes, then set head and tail to enqueued tcb and increment size
	if(list->size == 0) {
		list->head = tcb;
		list->tail = tcb;
		(list->size)++;
		
		#ifdef __DEBUG
		printListContents(list);
		printSchedulerStatus();
		printf("\ntcbList_enqueue: Exit\n");
		#endif
		return osNoError;
	}
	
	// Set current tail TCB to point to enqueued TCB
	list->tail->nextTcb = tcb;
	
	// Assign enqueued TCB to tail
	list->tail = tcb;
	
	// Increment size
	(list->size)++;
	
	#ifdef __DEBUG
	printListContents(list);
	printSchedulerStatus();
	printf("\ntcbList_enqueue: Exit\n");
	#endif
	return osNoError;
}

tcb_t *tcbList_dequeue(tcbList_t *list) {
	#ifdef __DEBUG
	printf("\ntcbList_dequeue: Enter\n");
	#endif
	// Return NULL if queue is empty
	tcb_t *returnTcb;
	
	if(list->head == NULL) {
		#ifdef __DEBUG
		printf("\ntcbList_dequeue: Exit Error, Empty Queue\n");
		#endif
		return NULL;
	}
	
	// Check if the list has only one element
	if(list->size == 1) {
		returnTcb = list->head;
		list->head = NULL;
		list->tail = NULL;
		(list->size)--;
		
		#ifdef __DEBUG
		printListContents(list);
		printSchedulerStatus();		
		printf("\ntcbList_dequeue: Exit");
		#endif
		return returnTcb;
	}
	
	// Set head to dequeued TCB's next TCB
	returnTcb = list->head;
	list->head = list->head->nextTcb;
	
	// Assign dequeued TCB's next to NULL
	returnTcb->nextTcb = NULL;
	(list->size)--;
	
	#ifdef __DEBUG
	printListContents(list);
	printSchedulerStatus();
	printf("\ntcbList_dequeue: Exit\n");
	#endif
	return returnTcb;
}

osError_t changeState(tcb_t *tcb, taskState_t newState) {
	
	taskState_t oldState = tcb->state;
	
	if(	// Task is running and gets blocked by mutex/semaphore
			((oldState == T_RUNNING) && (newState == T_BLOCKED)) ||
			// High priority task is unblocked
			((oldState == T_BLOCKED) && (newState == T_READY) && (tcb->priority > scheduler.currPriority)) ||
			// New task is created and it is higher priority than the current task
			((oldState == T_INACTIVE) && (newState == T_READY) && (tcb->priority > scheduler.currPriority))	) {
		runScheduler = true;
	}

	// Set the new state to the tcb
	tcb->state = newState;
	
	return osNoError;
}

void printSchedulerStatus(void) {
	printf("\nCurrent task: %d, State: %d, Current Priority: %d, [0]: %d, [1]: %d, [2]: %d, [3]: %d\n", 
				 scheduler.currTCB->tid, 
				 scheduler.currTCB->state,
				 scheduler.currTCB->priority,
				 scheduler.readyQueueList[0].size,
				 scheduler.readyQueueList[1].size,
				 scheduler.readyQueueList[2].size,
				 scheduler.readyQueueList[3].size);
}

osError_t contextSwitch(tcb_t *oldTCB, tcb_t *newTCB) {
	
	oldTCB->stackPointer = (uint32_t*)storeContext();
	restoreContext((uint32_t)newTCB->stackPointer);
	
	return osNoError;
}

tcbList_t *findNextTask(void) {
	#ifdef __DEBUG
	printf("\nfindNextTask: Enter\n");
	#endif
	
	// Scan through Task List to find Highest Priority
	priority_t priorityIndex = osPriorityHigh;
	while(scheduler.readyQueueList[priorityIndex].head == NULL) {
		priorityIndex--;
	}
	
	tcbList_t *nextQueue;
	nextQueue = &scheduler.readyQueueList[priorityIndex];
	#ifdef __DEBUG
	printf("\nfindNextTask: Exit\n");
	#endif
	return nextQueue;
}

void SysTick_Handler(void) {

	// Increment ms
  msTicks++;
	// Decrement countDown
	countDown--;
	
	
	// Check if scheduler is explicitly called
	if(runScheduler == true) {
		
		runScheduler = false;
		
		// Reset countdown
		countDown = timeSlice;
		// Set PENDSV
		SCB->ICSR |= SET_PENDSV; 
		return;
	}
	
	// Check if timeslice has ended
	if(countDown == 0) {
		// Reset countdown
		countDown = timeSlice;
		
		// Place task that just finished running to back of its queue
		tcb_t *prevTask;
		
		prevTask = scheduler.currTCB;
		tcbList_t *prevList = &scheduler.readyQueueList[prevTask->priority];
		
		// Change finished task to ready state
		changeState(prevTask, T_READY);

		tcbList_dequeue(prevList);
		tcbList_enqueue(prevList, prevTask);
			
		// Set PENDSV
		SCB->ICSR |= SET_PENDSV; 
		
		return;
	}
}

void PendSV_Handler(void) {
	printf("\nScheduler: Enter\n");
	printSchedulerStatus();
	
	// Create pointer to next task to run
	tcb_t *prevTask;
	tcb_t *nextTask;
	
	// Store previous task
	prevTask = scheduler.currTCB;
	
	// Find next task to run 
	tcbList_t *nextQueue = findNextTask();
	nextTask = nextQueue->head;
	
	// Context switch between old task and new
	contextSwitch(prevTask, nextTask);
	
	scheduler.currTCB = nextTask;
	changeState(scheduler.currTCB, T_RUNNING);
	scheduler.currPriority = scheduler.currTCB->priority;
	
	// Clear PENDSV
	SCB->ICSR |= (CLEAR_PENDSV);
	
	printSchedulerStatus();
	printf("\nScheduler: Exit\n");
}

void initScheduler(void) {
	tcb[0].tid = IDLE_ID;
	tcb[0].state = T_RUNNING;
	
	scheduler.currTCB = &tcb[0];
	scheduler.currPriority = osPriorityNone;
	scheduler.readyQueueList[osPriorityNone].size = 1;
	scheduler.readyQueueList[osPriorityNone].head = &tcb[0];
	scheduler.readyQueueList[osPriorityNone].tail = &tcb[0];
	
	scheduler.readyQueueList[osPriorityLow].size = 0;
	scheduler.readyQueueList[osPriorityMed].size = 0;
	scheduler.readyQueueList[osPriorityHigh].size = 0;
}
