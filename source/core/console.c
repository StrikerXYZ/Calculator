#include "console.h"

#include "platform.h"


void console_log_string(StringView in_string_view)
{
	platform_console_log(in_string_view.data, (U4)in_string_view.size);
}

void console_log_buffer(Buffer in_string_heap)
{
	platform_console_log((Char const*)in_string_heap.data, (U4)in_string_heap.size);
}

void console_log_U8(U8 in_number)
{
	Char buffer[U8_MAX_STRING_SIZE];
	Char* buffer_pointer = buffer + (U8_MAX_STRING_SIZE - 1);
	U2 string_size = 0;

	do
	{
		*buffer_pointer-- = (Char)('0' + (in_number % 10));
		in_number /= 10;
		string_size++;
	} while (in_number);

	platform_console_log(buffer_pointer + 1, string_size);
}

U4 console_count_command_line_arguments(const Char* in_command_line) 
{
	U4 count = 0;
	U1 in_arg = 0;
	while (*in_command_line) {
		if (*in_command_line == ' ' || *in_command_line == '\t') {
			in_arg = 0;
		}
		else if (!in_arg) {
			++count;
			in_arg = 1;
		}
		++in_command_line;
	}
	return count;
}

