
#include "Queue.h"
#include <string.h>

/*
 * PRIVATE DEFINITIONS
 */

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

/*
 * PRIVATE VARIABLES
 */

/*
 * PUBLIC FUNCTIONS
 */

void Queue_Init(Queue_t * queue, void * item_bfr, uint32_t item_size, uint32_t item_capacity)
{
	queue->count = 0;
	queue->index = 0;
	queue->items = item_bfr;
	queue->item_size = item_size;
	queue->capacity = item_capacity;
}

bool Queue_Push(Queue_t * queue, const void * item)
{
	if (queue->count < queue->capacity)
	{
		uint32_t index = queue->index;

		memcpy(queue->items + (index * queue->item_size), item, queue->item_size);

		queue->index = index + 1 < queue->capacity ? index + 1 : 0;
		queue->count += 1;
		return true;
	}
	return false;
}

bool Queue_Pop(Queue_t * queue, void * item)
{
	if (queue->count > 0)
	{
		int32_t index = queue->index - queue->count;
		if (index < 0) { index += queue->capacity; }

		memcpy(item, queue->items + (index * queue->item_size), queue->item_size);

		queue->count -= 1;
		return true;
	}
	return false;
}

void Queue_Clear(Queue_t * queue)
{
	queue->count = 0;
	queue->index = 0;
}

uint32_t Queue_Free(Queue_t * queue)
{
	return queue->capacity - queue->count;
}

uint32_t Queue_Count(Queue_t * queue)
{
	return queue->count;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * INTERRUPT ROUTINES
 */

