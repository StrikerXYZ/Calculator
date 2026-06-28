#pragma once

#include "types.h"
#include "memory.h"

typedef struct 
{
	U8 capacity;
	U8 size;
	Binary * data;
} Buffer;

typedef struct
{
	U8 size;
	Char const * data;
} StringView;

U8 buffer_find_terminator_position(Char const* cstring);

StringView string_create_stack(Char const * c_string);
Buffer buffer_create_heap(Arena * arena, Void const * c_string, U8 size);
StringView string_create_from_buffer(Buffer in_string);

U1 buffer_find_byte(U8* out_index, Binary const* data, U8 size, Binary in_byte);
U1 buffer_find_byte(U8* out_index, Binary const* data, U8 size, Binary in_byte);
U1 buffer_split(StringView* out_data, U4 in_max_strings, Binary const* in_data, U8 in_size, Binary in_delimiter);
void string_to_case_upper(Char * data, U8 size);
void string_to_case_lower(Char * data, U8 size);
U1 buffer_equals(Binary const* buffer_a, U8 size_a, Binary const* buffer_b, U8 size_b);

U8 buffer_convert_to_u8(Char const* buffer_data, U8 buffer_size);
StringView buffer_convert_from_u8(Char* buffer_data, U8 buffer_capacity, U8 value);
F8 buffer_convert_to_f8(Char const* buffer_data, U8 buffer_size);
StringView buffer_convert_from_f8(Char* buffer_data, U8 buffer_capacity, F8 value);
