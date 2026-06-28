#pragma once

#include "string.h"

void console_log_string(StringView in_string_view);
void console_log_buffer(Buffer in_string_heap);
void console_log_u8(U8 in_number);
U4 console_count_command_line_arguments(const Char* in_command_line);
