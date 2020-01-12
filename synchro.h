/*

	Header file for synchronization methods
	
	Author: Boris Kim

*/

#ifndef __SYNCHRO_H
#define __SYNCHRO_H

#include <LPC17xx.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "scheduler.h"

// Semaphore Methods
void osSemaphoreInit(sem_t *sem, uint32_t count);
osError_t osSemaphoreLend(sem_t *sem);
osError_t osSemaphoreReturn(sem_t *sem);

// Mutex Methods
void osMutexInit(mutex_t *mutex);
osError_t osMutexLock(mutex_t *mutex);
osError_t osMutexUnlock(mutex_t *mutex);

#endif //__SYNCHRO_H
