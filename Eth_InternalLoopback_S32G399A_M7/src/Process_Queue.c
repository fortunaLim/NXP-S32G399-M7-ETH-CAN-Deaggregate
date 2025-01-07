/*
 * Process_Queue.c
 *
 *  Created on: 2022. 6. 8.
 *      Author: J.H.Kim
 */

#include "Process_Queue.h"
//여기 수정함

extern char temp_string[30];
int time_offset2 = 8;

//큐 초기화
void init_queue(QueueType* q) {
	q->front = q->rear = 0;
}

//큐가 비어 있는지 확인
int is_empty(QueueType* q) {
	//printf_SysLog("empty");
	return (q->front == q->rear);
}

//큐가 가득 찼는지 확인
int is_full(QueueType* q) {
	//printf_SysLog("full\r\n");
	return (q->front == ((q->rear+1)%MAX_QUEUE_SIZE));
}

//큐가 가득 차 있는지 확인 후 삽입 연산
//OK
void enqueue(QueueType* q, uint8 **BufPtr) {
	uint8 *BufPtr2;
	if (is_full(q)) {
		//printf_SysLog("enqueue_full\r\n");
	}
	else {
		q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
		q->data[q->rear].BufPtr = *BufPtr;
		//memcpy(q->data[q->rear].BufPtr, BufPtr, sizeof(*BufPtr));
		time_offset2++;
	}
}

//큐가 비어 있는지 확인 후 삭제 연산
//OK
uint8 * dequeue(QueueType* q) {
	if (is_empty(q)==1) {
		//printf_SysLog("empty\r\n");
		return 0;
	}
	else {
		//printf_SysLog("dequeue\r\n");
		q->front = (q->front + 1) % MAX_QUEUE_SIZE;
		return q->data[q->front].BufPtr;
	}
}
