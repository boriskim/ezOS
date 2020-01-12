/*

	Test file to demonstrate capabilities of 
	available features.
	
	Author: Boris Kim

*/

#include <LPC17xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "ezOS.h"

// Testcase select macro
#define TESTCASE1

/*
 Demonstrates that the RTOS is capable of context switching
 
	- creates three tasks of high priority 
	- alternates between the three, demonstrating context switches
*/
#ifdef TESTCASE1
void testTask_1(void* arg) {
	while(true) {
		printf("Running Task %d\n", (uint32_t)arg);
	}
}

void testTask_2(void* arg) {
	while(true) {
		printf("Running Task %d\n", (uint32_t)arg);
	}
}

void testTask_3(void* arg) {
	while(true) {
		printf("Running Task %d\n", (uint32_t)arg);
	}
}

int main(void) {
	printf("Program Start\n\n");
	
	osInitialize();
	
	__disable_irq();
	
	osCreateTask(testTask_1, (void*)1, osPriorityHigh);
	osCreateTask(testTask_2, (void*)2, osPriorityHigh);
	osCreateTask(testTask_3, (void*)3, osPriorityHigh);
	
	__enable_irq();
	
	while(true) {
		printf("0 ");
	}
	return 0;
}
#endif



/*
 Demonstrates that the RTOS handles FPP scheduling
 
	- Creates three tasks, two of high priority and one of low
	- Low priority task never runs as there are two high priority
*/
#ifdef TESTCASE2
void testTask_1(void* arg) {
	while(true) {
		printf("Running Task %d, High priority\n", (uint32_t)arg);
	}
}

void testTask_2(void* arg) {
	while(true) {
		printf("Running Task %d, High Priority\n", (uint32_t)arg);
	}
}

void testTask_3(void* arg) {
	while(true) {
		printf("Running Task %d, Low Priority\n", (uint32_t)arg);
	}
}

int main(void) {
	printf("Program Start\n\n");
	
	osInitialize();
	
	__disable_irq();
	
	osCreateTask(testTask_1, (void*)1, osPriorityHigh);
	osCreateTask(testTask_2, (void*)2, osPriorityHigh);
	osCreateTask(testTask_3, (void*)3, osPriorityLow);
	
	__enable_irq();
	
	while(true) {
		printf("0 ");
	}
	return 0;
}
#endif



/*
 Demonstrates that the RTOS is capapble of handling blocking semaphores

	- Initially creates three tasks of High, Med, and Low priority
	- High and Med tasks are blocked by semaphore
	- Low task runs for 100 counts and releases Med semaphore
	- Med task released recognized by scheduler, immediately pre-empts so higher priority task can run
	- Med task runs for 100 counts and releases High semaphore
	- High task released recognized by scheduler, pre-empts to run and runs indefinitely
*/

#ifdef TESTCASE3

sem_t blockHigh;
sem_t blockMed;

void testTask_1(void* arg) {
	osSemaphoreLend(&blockHigh);
	while(true) {
		printf("High Task unblocked and running\n");
	}
}

void testTask_2(void* arg) {
	uint32_t counter = 0;
	
	osSemaphoreLend(&blockMed);
	while(true) {
		counter++;
		
		if(counter == 100) {
			osSemaphoreReturn(&blockHigh);
		}
		printf("Med Task unblocked and running\n");
	}
}

void testTask_3(void* arg) {
	uint32_t counter = 0;
	
	while(true) {
		counter++;
		
		if(counter == 100) {
			osSemaphoreReturn(&blockMed);
		}
		
		printf("Low Task is running\n");
	}
}

int main(void) {
	printf("Program Start\n\n");
	
	osSemaphoreInit(&blockHigh, 0);
	osSemaphoreInit(&blockMed, 0);
	
	
	osInitialize();
	
	__disable_irq();
	
	osCreateTask(testTask_3, NULL, osPriorityLow);
	osCreateTask(testTask_2, NULL, osPriorityMed);
	osCreateTask(testTask_1, NULL, osPriorityHigh);

	__enable_irq();
	
	while(true) {
		printf("Running Idle Task\n");
	}
	return 0;
}
#endif



/*
 Demonstrates that the RTOS is able to support mutex owner test on release
	- Create two tasks of equal priority
	- Task 1 starts off by locking the mutex. It attempts to acquire it again immediately after and gets rejected
	- At 100 counts, Task 2 tries to release the mutex but fails because it is owned by Task 1
	- At 150 counts, Task 2 tries to lock the mutex and gets blocked. Pre-empts Task 1 to run
	- At 200 counts, Task 1 releases the mutex, allowing Task 2 to acquire the mutex and from there both run indefinitely
*/
#ifdef TESTCASE4

mutex_t ownerMutex;

void testTask_1(void* arg) {
	osMutexLock(&ownerMutex);
	printf("Task 1 acquired the mutex, status is: %d\n", (uint8_t)ownerMutex.available);
	osMutexLock(&ownerMutex);
	
	uint32_t counter = 0;
	while(true) {
		counter++;
		
		if(counter == 200) {
			osMutexUnlock(&ownerMutex);
			printf("Task 1 has released the mutex, status is: %d\n", (uint8_t)ownerMutex.available);
		}
		printf("Task 1 is running, counter: %d\n", counter);
	}
}

void testTask_2(void* arg) {
	uint32_t counter = 0;	
	
	while(true) {
		counter++;
		
		if(counter == 100) {
			printf("Task 2 is attempting to release the mutex\n");
			osMutexUnlock(&ownerMutex);
		}
		
		if(counter == 150) {
			printf("Task 2 is attempting to acquire the mutex, will get blocked\n");
			osMutexLock(&ownerMutex);
			printf("Task 1 released the mutex and allowed Task 2 to run\n");
		}
		printf("Task 2 is running, counter: %d\n", counter);
	}
}

int main(void) {
	printf("Program Start\n\n");
	
	osMutexInit(&ownerMutex);
	
	osInitialize();
	
	__disable_irq();
	
	osCreateTask(testTask_1, NULL, osPriorityHigh);
	osCreateTask(testTask_2, NULL, osPriorityHigh);

	__enable_irq();
	
	while(true) {
		printf("Running Idle Task\n");
	}
	return 0;
}
#endif



/*
 Demonstrates that the RTOS handles priority inheritance of mutexes
	- Create 4 tasks, 2 high, 1 med, and 1 low. Med should never run
	- Initialize the mutex to be held onto by the low task before any task begins
	- Allow 2 high tasks to run round-robin until one of them tries to acquire the mutex and gets blocked
	- Low priority task gets promoted and is allowed to run along the other high priority task until it releases the mutex
	- Once low priority task releases the mutex, it is removed from the high priority queue
	- Blocked high task becomes unblocked and runs round-robin with the other high priority task indefinitely.
*/
#ifdef TESTCASE5

mutex_t priorityMutex;

void testTask_1(void* arg) {
	uint32_t counter = 0;
	
	while(true) {
		counter++;
		
		if(counter == 100) {
			printf("Release mutex\n");
			osMutexUnlock(&priorityMutex);
		}

		printf("Task Low is running, counter: %d\n", counter);
	}
}

void testTask_2(void* arg) {
	while(true) {
		printf("Task Med should never run, if you see this, you messed up\n");
	}
}

void testTask_3(void* arg) {
	uint32_t counter = 0;	

	while(true) {
		counter++;
		
		if(counter == 100) {
			printf("High priority task attempting to acquire mutex, will be blocked\n");
			osMutexLock(&priorityMutex);
		}
		
		printf("Task High is running, counter: %d\n", counter);
	}
}

void testTask_4(void* arg) {
	while(true) {
		printf("Other High priority task is running\n");
	}
}

int main(void) {
	printf("Program Start\n\n");
	
	osMutexInit(&priorityMutex);
	
	osInitialize();
	
	__disable_irq();
	
	osCreateTask(testTask_1, NULL, osPriorityLow);
	
	tcbList_t *nextQueue = findNextTask();
	tcb_t *lowPriorityTcb;
	
	lowPriorityTcb = nextQueue->head;
	
	priorityMutex.owner = lowPriorityTcb;
	priorityMutex.available = false;
	priorityMutex.originalPriority = osPriorityLow;

	osCreateTask(testTask_2, NULL, osPriorityMed);
	osCreateTask(testTask_3, NULL, osPriorityHigh);
	osCreateTask(testTask_4, NULL, osPriorityHigh);

	__enable_irq();
	
	while(true) {
		printf("Running Idle Task\n");
	}
	return 0;
}
#endif
