#pragma once

#include "types.h"
#include "file.h"
#include "memory.h"
#include "renderer.h"

typedef struct 
{
	U4 width;
	U4 height;
} WindowInfo;
WindowInfo platform_get_window_info(Handle window_handle);

typedef struct
{
	F4 mouse_x;
	F4 mouse_y;

	U1 mouse_left_pressed;
	U1 mouse_left_held;
	U1 mouse_left_released;

	Char chars[32];
	U4 char_count;
	U1 backspace;
	U1 enter;
	U1 escape;
} InputState;

void bebug_breakpoint(void);

void platform_console_log(const Char* data, U8 size);

U8 platform_console_log_file(FileReadHandle file_handle);

U4 platform_console_read(Char* data, U8 size);

U4 platform_console_peek(void);

void platform_console_clear(void);

const Char* GetCommandLineArgs(void);

void platform_reserve_pages(Binary** memory_ptr, U8 size);

void platform_commit_pages(Binary* memory_ptr, U8 size);

FileReadHandle platform_file_open_read(const Char* path);
FileWriteHandle platform_file_open_write(const Char* path);
void platform_file_close(FileReadHandle file_handle);

U1 platform_file_read(FileReadHandle file_handle, Char* buffer, U4 buffer_size, U4* bytes_read);
U1 platform_file_write(FileWriteHandle file_handle, const Char* buffer, U4 buffer_size, U4* bytes_written);

void platform_window_create(const Char* title, U4 width, U4 height);
U1 platform_window_process_messages(void);
void platform_window_destroy(void);

void run(
	Arena* arena, 
	InputState* input_state, 
	RendererContext renderer_context, 
	U4 window_width, U4 window_height
);
