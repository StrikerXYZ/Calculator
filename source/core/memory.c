#include "memory.h"

#include "debug.h"
#include "platform.h"

void memory_arena_create(Arena* arena, U8 max_size)
{
	arena->max_size = max_size;
	arena->used_size = 0;
	platform_reserve_pages(&arena->memory, max_size);
}

Binary* memory_arena_allocate(Arena* arena, U8 request_size)
{
	U8 old_size = arena->used_size;
	U8 new_size = old_size + request_size;

	DEBUG_RUNTIME_ASSERT(new_size <= arena->max_size, "Arena out of memory");
	arena->used_size = new_size;
	platform_commit_pages(arena->memory, arena->used_size);

	return arena->memory + old_size;
}

void memory_zero(void* destination, U8 size)
{
	Binary* destination_bytes = (Binary*)destination;
	for (U8 i = 0; i < size; ++i) {
		*(destination_bytes++) = 0u;
	}
}

void memory_copy(void* destination, void const* source, U8 size)
{
	Binary* destination_bytes = (Binary*)destination;
	Binary const* source_bytes = (Binary const*)source;
	for (U8 i = 0; i < size; ++i) {
		destination_bytes[i] = source_bytes[i];
	}
}
