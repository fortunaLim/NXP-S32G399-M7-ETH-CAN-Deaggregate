/*
 * Process_Queue_sh.h
 *
 *  Created on: 2022. 6. 8.
 *      Author: Jonghun Kim
 */

#ifndef PROCESS_QUEUE_H_
#define PROCESS_QUEUE_H_

#define MAX_QUEUE_SIZE 8

#include "Mcu.h"
//#include "Port.h"
#include "Eth.h"
#include "OsIf.h"
#include "Platform.h"
#include "string.h"
/*-------- struct -----------*/
typedef struct BufPtr
{
	uint8 *BufPtr;
}BufPtr_Q;

typedef struct _QueueType {
	BufPtr_Q data[MAX_QUEUE_SIZE];
	uint32 front, rear;
}QueueType;



/*-------- function -----------*/
void init_queue(QueueType* q);
int is_empty(QueueType* q);
int is_full(QueueType* q);
void enqueue(QueueType* q, uint8 **BufPtr);
uint8 * dequeue(QueueType* q);
void print_queue(QueueType* q);


#endif /* PROCESS_QUEUE_H_ */
