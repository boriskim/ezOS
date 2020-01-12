/*

	Main source file for ezOS initializations
	
	Author: Boris Kim

*/

#include "ezOS.h"

/**********************************************GLOBAL VARIABLES********************************************/
// scheduler
extern scheduler_t scheduler;

// TCB's
extern tcb_t tcb[NUM_TCB];
extern tcb_t main_tcb;
/**********************************************GLOBAL VARIABLES********************************************/

osError_t osInitialize(void) {
	__disable_irq();
	
	// Configure SysTick interrupt
	SysTick_Config(SystemCoreClock/1000);
	
	#ifdef __DEBUG
	printf("\nosInitialize: Enter\n");
	printGlobalLocations();
	#endif
	uint32_t *VECTOR_ZERO;
	VECTOR_ZERO = 0x0;
	const uint32_t KIBI = 0x100;
	const uint32_t PSP_ENABLE = 1<<1;
		
	main_tcb.stackPointer = (uint32_t*)*VECTOR_ZERO;
	main_tcb.stackBaseAddress = main_tcb.stackPointer;
	main_tcb.stackOverflowAddress = main_tcb.stackPointer - (2*KIBI);
	main_tcb.priority = osPriorityNone;
	main_tcb.state = T_INACTIVE;
	main_tcb.tid = 99;
  
	uint32_t* stackLocater = main_tcb.stackPointer - 2*KIBI;
	
	#ifdef __DEBUG
	printf("Main Stack is at %p and overflow at %p\n", main_tcb.stackBaseAddress, main_tcb.stackOverflowAddress);
	#endif
	
	for(int32_t stackCount = 0; stackCount < NUM_TCB; stackCount++) {
		tcb[stackCount].stackPointer = (stackLocater - stackCount*KIBI);
		tcb[stackCount].stackBaseAddress = (stackLocater - stackCount*KIBI);
		tcb[stackCount].stackOverflowAddress = (stackLocater - (KIBI*(stackCount + 1)));
		tcb[stackCount].priority = osPriorityNone;
		tcb[stackCount].state = T_INACTIVE;
		tcb[stackCount].nextTcb = NULL;
		tcb[stackCount].tid = 0;
		#ifdef __DEBUG
		printf("Stack %d is at %p and overflow at %p, TCB location: %p\n", stackCount, tcb[stackCount].stackBaseAddress, tcb[stackCount].stackOverflowAddress, &tcb[stackCount]);
		#endif
	}
	
	// Copy the Main Stack contents to the process stack of the new main() task, at tcb[0]
	uint32_t topMainStack = __get_MSP();
	
	uint32_t topNewMainStack = topMainStack - 2*4*KIBI;

	#ifdef __DEBUG
	printf("\nMSP was at: %p\n", (uint32_t*)__get_MSP());;
	#endif	

	for(uint32_t *mainStackLoc = main_tcb.stackBaseAddress; (uint32_t)mainStackLoc >= topMainStack; mainStackLoc--) {
		uint32_t *copyPointer = mainStackLoc - (2*KIBI);		
		*copyPointer = *mainStackLoc;
		#ifdef __DEBUG
		if(mainStackLoc == (uint32_t*)topMainStack) {
			printf("\ncopyPointer is at: %p\n", copyPointer);
		}
		#endif
	}
	
	// Set MSP to base address of main stack
	__set_MSP((uint32_t)main_tcb.stackBaseAddress);
	
	#ifdef __DEBUG
	printf("\nMSP is now at: %p\n", (uint32_t*)__get_MSP());;
	#endif
	
	// Set PSP to the top of main() task
	__set_PSP(topNewMainStack);
	
	#ifdef __DEBUG
	printf("PSP is now at: %p\n", (uint32_t*)__get_PSP());
	#endif
	
	// Switch from using the MSP to the PSP
	__set_CONTROL((uint32_t)__get_CONTROL() | PSP_ENABLE);

	// Initialize the scheduler with Idle Task
	initScheduler();
	
	#ifdef __DEBUG
	printf("\nosInitialize: Exit\n");
	#endif
	__enable_irq();
	return osNoError;
}

osError_t osCreateTask(osThreadFunc_t functionPointer, void* functionArgument, priority_t priority) {
	#ifdef __DEBUG
	//printf("\nosCreateTask: Enter\n");
	#endif
	static uint32_t taskNumber = 1;
	const uint32_t PSR_VAL = 0x01000000;
	
	tcb[taskNumber].tid = taskNumber;
	tcb[taskNumber].priority = priority;
	changeState(&tcb[taskNumber], T_READY);
	
	#ifdef __DEBUG
	printTcbContents(&tcb[taskNumber]);
	#endif
	
	// Add register values to stack starting from PSR
	tcb_push(&tcb[taskNumber], PSR_VAL);
	
	// Set PC -> Function Pointer
	tcb_push(&tcb[taskNumber], (uint32_t)functionPointer);
	
	// Fill LR - R4 with placeholder bit
	for(uint32_t count = 0; count < 14; count++) {
		if(count == 5) {
			// Set R0 -> Function Argument
			tcb_push(&tcb[taskNumber], (uint32_t)(functionArgument));
		}
		else {
			tcb_push(&tcb[taskNumber], 0x01);
		}
	}
	
	#ifdef __DEBUG
	printf("osCreateTask: Right before schedule task:\n");
	printTcbContents(&tcb[taskNumber]);
	#endif
	
	tcb_t *newTask = &tcb[taskNumber];
	tcbList_t *taskQueue = &scheduler.readyQueueList[priority];
	
	tcbList_enqueue(taskQueue, newTask);
	
	#ifdef __DEBUG
	printSchedulerStatus();
	printf("\nosCreateTask: Exit, created Tid: %d\n", tcb[taskNumber].tid);
	#endif
	
	// Increment Task Number
	taskNumber++;
	return osNoError;
}

void osPrintError(osError_t error) {
	
	printf("Error Code: ");
	switch(error) {
		
		case osNoError :
			printf("No Error\n");
			break;
		
		case osErrorOverflow :
			printf("Stack Overflow\n");
			break;
		
		case osErrorInv :
			printf("Invalid Call\n");
			break;
		
		case osErrorPerm :
			printf("Permission Error\n");
			break;
		
		case osErrorEmp :
			printf("Element Empty\n");
			break;
		
		default : 
			printf("Invalid Error Code\n");
	}
}
