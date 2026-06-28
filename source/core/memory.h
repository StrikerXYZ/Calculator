#pragma once

#include "types.h"

typedef struct {
	U8 max_size;
	U8 used_size;
	Binary* memory;
} Arena;

void memory_arena_create(Arena* arena, U8 max_size);

Binary* memory_arena_allocate(Arena* arena_handle, U8 request_size);

void memory_zero(void* destination, U8 size);
void memory_copy(void* destination, void const* source, U8 size);
