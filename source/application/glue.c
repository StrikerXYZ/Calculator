#include "glue.h"

#include "../core/console.h"
#include "../core/string.h"
#include "../core/platform.h"


Buffer console_read_string(Char* buffer, U4 size)
{
	Buffer string;
	string.data = (Binary*)buffer;
	string.size = platform_console_read(buffer, size);
	return string;
}

Buffer console_read_buffer(Arena* arena, const U8 max_characters)
{
	Buffer string;
	string.capacity = max_characters;
	string.data = (Binary*)memory_arena_allocate(arena, max_characters);
	string.size = platform_console_read((Char*)string.data, max_characters);
	return string;
}

U8 console_read_U8(void)
{
	Char buffer[U8_MAX_STRING_SIZE];
	U4 bytes_read = platform_console_read(buffer, U8_MAX_STRING_SIZE);
	return buffer_convert_to_u8(buffer, bytes_read);
}
