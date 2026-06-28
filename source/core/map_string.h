#pragma once

#include "string.h"

typedef struct
{
	U4 keys[1024];
	StringView values[1024];
	U4 size;
} MapString;


U4 hash_fnv1a(StringView string);

U4 map_hash(StringView key);
void map_add(MapString* map, StringView key, StringView value);
U1 map_get(MapString* map, StringView key, StringView* out_value);
U1 map_contains(MapString* map, StringView key);
