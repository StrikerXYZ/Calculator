#include "string.h"
#include "memory.h"
#include "debug.h"

U8 buffer_find_terminator_position(Char const * c_string)
{
	U8 result = 0;
	while (c_string[result] != '\0')
		++result;
	return result;
}

StringView string_create_view(Char const* c_string, U8 size)
{
	StringView result;
	result.size = size;
	result.data = c_string;
	return result;
}

StringView string_create_stack(Char const* c_string)
{
	const U8 size = buffer_find_terminator_position(c_string);
	const StringView result = string_create_view(c_string, size);
	return result;
}

Buffer buffer_create_heap(Arena * arena, Void const * c_string, U8 size)
{
	Buffer result;
	result.size = size;
	result.data = (Binary*)memory_arena_allocate(arena, size);
	memory_copy(result.data, c_string, size);
	return result;
}

StringView string_create_from_buffer(Buffer buffer)
{
	StringView string_view;
	string_view.size = buffer.size;
	string_view.data = (Char const*)buffer.data;
	return string_view;
}

void string_to_case_upper(Char * data, U8 size)
{
	Char const upper_offset = 'A' - 'a';

	// Convert to upper case
	for (U4 i = 0; i < size; ++i)
	{
		Char character = data[i];
		if (character >= 'a' && character <= 'z')
		{
			data[i] = character + upper_offset;
		}
	}
}

void string_to_case_lower(Char * data, U8 size)
{
	Char const lower_offset = 'a' - 'A';

	// Convert to lower case
	for (U4 i = 0; i < size; ++i)
	{
		Char character = data[i];
		if (character >= 'A' && character <= 'Z')
		{
			data[i] = character + lower_offset;
		}
	}
}

U4 string_count_character(Char * data, U8 size, Char in_char)
{
	U4 count = 0;
	for (U4 i = 0; i < size; ++i)
	{
		count += (data[i] == in_char);
	}
	return count;
}

U1 buffer_find_byte(U8* out_index, Binary const * data, U8 size, Binary in_byte)
{
	for (U4 i = 0; i < size; ++i)
	{
		if (data[i] == in_byte)
		{
			*out_index = i;
			return 1;
		}
	}
	*out_index = size;
	return 0;
}

U1 buffer_split(StringView * out_data, U4 in_max_strings, Binary const * in_data, U8 in_size, Binary in_delimiter)
{
	U1 result = 1;
	U4 strings_index = 0;

	for (; strings_index < in_max_strings; ++strings_index)
	{
		U8 start_index = (strings_index == 0)? 0 : out_data[strings_index - 1].size + 1;
		in_data += start_index;
		in_size -= start_index;
		out_data[strings_index].data = (Char const *)in_data;
		if (!buffer_find_byte(&out_data[strings_index].size, in_data, in_size, in_delimiter))
		{
			result = (strings_index + 1 == in_max_strings);
			break;
		}
	}

	return result;
}

U1 buffer_equals(Binary const* buffer_a, U8 size_a, Binary const* buffer_b, U8 size_b)
{
	if (size_a != size_b)
		return 0;

	for (U8 i = 0; i < size_a; ++i)
	{
		if (buffer_a[i] != buffer_b[i])
			return 0;
	}

	return 1;
}

U8 buffer_convert_to_u8(Char const* buffer_data, U8 buffer_size)
{
	U8 result = 0;

	for (U4 i = 0; i < buffer_size; ++i)
	{
		Char value = buffer_data[i];

		if (value < '0' || value > '9')
		{
			return result;
		}
		result = result * 10u + (U8)(value - '0');
	}

	return result;
}

StringView buffer_convert_from_u8(Char* buffer_data, U8 buffer_capacity, U8 value)
{
	Char* buffer_pointer = buffer_data + buffer_capacity - 1;
	U2 string_size = 0;

	do
	{
		*buffer_pointer-- = (Char)('0' + (value % 10));
		value /= 10;
		string_size++;
		DEBUG_RUNTIME_ASSERT(string_size <= buffer_capacity, "Expected: enough size provided to generate character sequence");
	} while (value);

	StringView result;
	result.data = buffer_pointer + 1;
	result.size = string_size;
	return result;
}

F8 buffer_convert_to_f8(Char const * buffer_data, U8 buffer_size)
{
	DEBUG_RUNTIME_ASSERT(buffer_size > 0, "Expected: string is not empty");

	U4 cursor = 0;
	U1 is_negative = (buffer_size > 1 && buffer_data[cursor] == '-');

	if (is_negative) cursor++;

	F8 result = 0;
	while (
		cursor < buffer_size
		&& buffer_data[cursor] >= '0'
		&& buffer_data[cursor] <= '9'
		)
	{
		Char value = buffer_data[cursor];
		DEBUG_RUNTIME_ASSERT(value >= '0' && value <= '9', "Expected: string contains only digits");
		result = result * 10u + (F8)(value - '0');
		cursor += 1;
	}

	if (cursor < buffer_size && buffer_data[cursor] == '.')
	{
		++cursor;
		F8 factor = 0.1;
		while (cursor < buffer_size
			&& buffer_data[cursor] >= '0'
			&& buffer_data[cursor] <= '9')
		{
			result += (F8)(buffer_data[cursor] - '0') * factor;
			factor *= 0.1;
			cursor += 1;
		}
	}

	result = is_negative ? -result : result;
	return result;
}

StringView buffer_convert_from_f8(Char* buffer_data, U8 buffer_capacity, F8 value)
{
	DEBUG_RUNTIME_ASSERT(buffer_capacity > 0, "Expected: buffer size is greater than 0");
	U4 cursor = 0;

	if (value < 0.0)
	{
		DEBUG_RUNTIME_ASSERT(cursor < buffer_capacity, "Expected: buffer has enough space for negative sign");
		buffer_data[cursor++] = '-';
		value = -value;
	}

	U8 int_part = (U8)value;
	F8 frac_part = value - (F8)int_part;

	if (int_part == 0)
	{
		DEBUG_RUNTIME_ASSERT(cursor < buffer_capacity, "Expected: buffer has enough space for integer part");
		buffer_data[cursor++] = '0';
	}
	else
	{
		Char int_buffer[U8_MAX_STRING_SIZE];
		U4 int_length = 0;
		U8 n = int_part;
		while (n > 0 && int_length < U8_MAX_STRING_SIZE)
		{
			int_buffer[int_length++] = '0' + (Char)(n % 10);
			n /= 10;
		}
		for (U4 i = 0; i < int_length && cursor < buffer_capacity; ++i)
			buffer_data[cursor++] = int_buffer[int_length - 1 - i];
	}
	if (frac_part > 1e-9 && cursor < buffer_capacity)
	{
		DEBUG_RUNTIME_ASSERT(cursor < buffer_capacity, "Expected: buffer has enough space for decimal point");
		buffer_data[cursor++] = '.';
		Char frac_buffer[10];
		U4 frac_length = 0;
		F8 factor = frac_part;
		for (U4 i = 0; i < 8 && cursor < buffer_capacity; ++i)
		{
			factor *= 10;
			U1 digit = (U1)(factor);
			DEBUG_RUNTIME_ASSERT(digit <= 9, "Expected: digit is between 0 and 9");
			buffer_data[cursor++] = '0' + digit;
			factor -= digit;
			if (factor <= 1e-9) break;
		}
	}

	StringView result;
	result.data = buffer_data;
	result.size = cursor;
	return result;
}
