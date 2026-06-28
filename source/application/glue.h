#pragma once

#include "../core/types.h"
#include "../core/string.h"
#include "../core/memory.h"

U8 console_read_U8(void);
Buffer console_read_string(char* buffer, U4 size);
Buffer console_read_buffer(Arena* arena, const U8 max_characters);
