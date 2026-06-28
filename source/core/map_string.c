#include "map_string.h"

#include "debug.h"
#include "random.h"

U4 hash_fnv1a(StringView string)
{
	U4 hash = 2166136261u;

	for (U4 index = 0; index < string.size; ++index)
	{
		hash ^= (unsigned char)(string.data[index]);
		hash *= 16777619u;
	}

	return hash;
}

U4 map_hash(StringView key)
{
	return hash_fnv1a(key);
}

void map_add(MapString* map, StringView key, StringView value)
{
	U4 hash = map_hash(key);

	DEBUG_RUNTIME_ASSERT(hash, "Expected: hash cannot be 0");
	DEBUG_RUNTIME_ASSERT(map->size < 1024, "Expected: map has space");

	U4 index = hash % 1024;

	while (map->keys[index])
	{
		index++;
	}

	map->keys[index] = hash;
	map->values[index] = value;

	map->size++;
}

U1 map_get(MapString* map, StringView key, StringView* out_value)
{
	U4 hash = map_hash(key);

	DEBUG_RUNTIME_ASSERT(hash, "Expected: hash cannot be 0");

	U4 index = hash % 1024;

	while (map->keys[index] != 0)
	{
		if (map->keys[index] == hash)
		{
			*out_value = map->values[index];
			return 1;
		}
	}

	return 0;
}

U1 map_contains(MapString* map, StringView key)
{
	U4 hash = map_hash(key);

	DEBUG_RUNTIME_ASSERT(hash, "Expected: hash cannot be 0");

	U4 index = hash % 1024;

	while (map->keys[index] != 0)
	{
		if (map->keys[index] == hash)
		{
			return 1;
		}
	}

	return 0;
}


internal void array_shuffle(XOShiro256State* random_state, U1* in_array, U1 array_count)
{
	if (array_count)
	{
		for (U1 i = 0; i < array_count - 1; ++i)
		{
			U1 swap_with_index = (U1)GetRandomNumberInRange(random_state, i, array_count - 1);
			if (i == swap_with_index) continue;
			U1 swap_temp = in_array[swap_with_index];
			in_array[swap_with_index] = in_array[i];
			in_array[i] = swap_temp;
		}
	}
}

internal U1 array_contains(U1* in_array, U1 array_count, U1 value)
{
	for (U1 i = 0; i < array_count; ++i)
	{
		if (in_array[i] == value) return 1;
	}
	return 0;
}


