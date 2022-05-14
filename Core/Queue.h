#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

/*
 * PUBLIC FUNCTIONS
 */

typedef struct {
	void * items;
	uint32_t item_size;
	uint32_t capacity;
	uint32_t count;
	uint32_t index;
} Queue_t;


void Queue_Init(Queue_t * queue, void * buffer, uint32_t item_size, uint32_t item_capacity);
bool Queue_Push(Queue_t * queue, const void * item);
bool Queue_Pop(Queue_t * queue, void * item);
void Queue_Clear(Queue_t * queue);
uint32_t Queue_Free(Queue_t * queue);
uint32_t Queue_Count(Queue_t * queue);

/*
 * EXTERN DECLARATIONS
 */

#endif //QUEUE_H
