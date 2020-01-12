/*

	Main header file for ezOS initializations
	
	Author: Boris Kim

*/

#ifndef EZOS_H
#define EZOS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <LPC17xx.h>

#include "synchro.h"

/**********************************************RTOS FUNCTIONS**********************************************/
// Initialization method
osError_t osInitialize(void);

// Task creation method
osError_t osCreateTask(osThreadFunc_t functionPointer, void* functionArgument, priority_t priority);

// Error print method
void osPrintError(osError_t error);
/**********************************************RTOS FUNCTIONS**********************************************/

#endif //EZOS_H
