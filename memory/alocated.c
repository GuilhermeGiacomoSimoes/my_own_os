#include<stdint.h>
#include<stdio.h>

#include "alocated.h"

#define NULL_POINT ((void*)0)
#define DYNAMIC_MEM_TOTAL_SIZE 4*1024
#define DYNAMIC_MEM_NODE_SIZE sizeof(dynamic_mem_node) /// 16
													   
typedef struct dynamic_mem_node {
	uint32_t size;
	int used;
	struct dynamic_mem_node *next;
	struct dynamic_mem_node *prev;
} dynamic_mem_node_t;

static uint8_t dynamic_mem_area[DYNAMIC_MEM_TOTAL_SIZE];
static dynamic_mem_node_t *dynamic_mem_start;

void __init_dynamic_mem()
{
	dynamic_mem_start = (dynamic_mem_node_t *) dynamic_mem_area;
	dynamic_mem_start->size = DYNAMIC_MEM_TOTAL_SIZE - 16; /// this 16 is dynamic memory node size
	dynamic_mem_start->next = NULL_POINT;
	dynamic_mem_start->prev = NULL_POINT;
}

void* __find_best_mem_block(dynamic_mem_node_t 
		*dynamic_mem, size_t size)
{
	dynamic_mem_node_t *best_mem_block = (dynamic_mem_node_t *) NULL_POINT;
	uint32_t best_mem_block_size = DYNAMIC_MEM_TOTAL_SIZE + 1;

	dynamic_mem_node_t *current_mem_block = dynamic_mem;
	while(current_mem_block) {
		if((!current_mem_block->used) &&
			current_mem_block->size >= (size + 16) &&
			(current_mem_block->size <= best_mem_block_size)) {
				best_mem_block = current_mem_block;
				best_mem_block_size = current_mem_block->size;
			}

		current_mem_block = current_mem_block->next;
	}

	return best_mem_block;
}

void *k_malloc(size_t size)
{
	dynamic_mem_node_t *best_mem_block = 
		(dynamic_mem_node_t *) __find_best_mem_block(dynamic_mem_start, size);

	if(best_mem_block != NULL_POINT) {
		best_mem_block->size = best_mem_block->size - size - 16; 
		dynamic_mem_node_t *mem_node_allocate 
			= (dynamic_mem_node_t *) (((uint8_t *) best_mem_block) + 16 + 
										best_mem_block->size);
		mem_node_allocate->size = size;
		mem_node_allocate->used = 1;
		mem_node_allocate->next = best_mem_block->next;
		mem_node_allocate->prev = best_mem_block;

		if(best_mem_block->next != NULL_POINT) {
			best_mem_block->next->prev = mem_node_allocate;
		}
		best_mem_block->next = mem_node_allocate;

		return (void *) ((uint8_t *) mem_node_allocate + 16);
	}

	return NULL_POINT;
}

void* __merge_next_node_into_current(
	dynamic_mem_node_t* current_mem_node)
{
	dynamic_mem_node_t *next_mem_node = current_mem_node->next;
	if(next_mem_node != NULL_POINT && !next_mem_node->used) {
		current_mem_node->size += current_mem_node->next->size;
		current_mem_node->size += 16;

		current_mem_node->next = current_mem_node->next->next;
		if(current_mem_node->next != NULL_POINT) 
			current_mem_node->next->prev = current_mem_node;
	}

	return current_mem_node;
}

void* __merge_current_node_into_previous(
	dynamic_mem_node_t* current_mem_node)
{
	dynamic_mem_node_t *prev_mem_node = current_mem_node->prev;
	if(prev_mem_node != NULL_POINT && !prev_mem_node->used) {
		prev_mem_node->size += current_mem_node->size;
		prev_mem_node->size += 16;

		prev_mem_node->next = current_mem_node->next;
		if(current_mem_node->next != NULL_POINT) 
			current_mem_node->next->prev = prev_mem_node;	
	}
}

void mem_free(void *p) 
{
	if(p == NULL_POINT) return;

	dynamic_mem_node_t *current_mem_node 
		= (dynamic_mem_node_t *) ((uint8_t *) p - 16);

	if(current_mem_node == NULL_POINT) return;

	current_mem_node->used = 0;

	current_mem_node = __merge_next_node_into_current(current_mem_node);
	__merge_current_node_into_previous(current_mem_node);
}

