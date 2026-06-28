// Copyright (c) 2026 Striker

#include "../core/types.h"
#include "../core/string.h"

#if FEATURE_DEBUG
#include "../test/test.h"
#endif

#if PLATFORM_WINDOWS

#include "platform_windows.h"

#include "../core/renderer.h"
#include "../core/platform.h"
#include "../core/memory.h"
#include "../core/math.h"

typedef void* Handle;
typedef DWORD DWord;
typedef COORD Coord;
typedef PCONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
typedef HINSTANCE Instance;
typedef WNDCLASSA WindowClass;
typedef HWND WindowHandle;

global U1 window_active = 1;
global InputState input_state = {0};
global Arena g_arena;
global RendererContext g_renderer_context;

// Minimal __chkstk for x64
void __chkstk(void) 
{
	// Currently not handling large stack allocations (4096+ bytes : page size) 
	//__debugbreak();
}

void* memset(void* dest, int c, unsigned long long count)
{
	unsigned char* d = (unsigned char*)dest;
	for (unsigned long long i = 0; i < count; ++i)
		d[i] = (unsigned char)c;
	return dest;
}

void* memmove(void* dest, const void* src, size_t n)
{
	unsigned char* d = (unsigned char*)dest;
	const unsigned char* s = (const unsigned char*)src;

	if (d == s || n == 0)
		return dest;

	// If dest is before src, we can copy forward safely
	if (d < s) {
		for (size_t i = 0; i < n; i++) {
			d[i] = s[i];
		}
	}
	// If dest is after src, copy backwards to avoid overwrite
	else {
		for (size_t i = n; i > 0; i--) {
			d[i - 1] = s[i - 1];
		}
	}

	return dest;
}

void* memcpy(void* dest, const void* src, unsigned long long count)
{
	memory_copy(dest, src, count);
	return dest;
}

size_t strlen(char const* str)
{
	size_t n = 0;
	while (str[n] != 0) ++n;
	return n;
}

internal W64(LRESULT) window_procedure(WindowHandle window_handle, U4 message_hash, U8 wparam, S8 lparam)
{
	switch (message_hash)
	{
		case WM_CLOSE:
		{
			DestroyWindow(window_handle);
		} break;
		case WM_DESTROY:
		{
			window_active = 0;
			PostQuitMessage(0);
			return 0;
		} break;
		case WM_SIZE:
		{
		} break;
		case WM_MOUSEMOVE:
		{
			input_state.mouse_x = (F4)(S2)(lparam & 0xFFFF);
			input_state.mouse_y = (F4)(S2)((lparam >> 16) & 0xFFFF);
		} break;
		case WM_LBUTTONDOWN:
		{
			input_state.mouse_left_pressed = 1;
			input_state.mouse_left_held = 1;
		} break;
		case WM_LBUTTONUP:
		{
			input_state.mouse_left_released = 1;
			input_state.mouse_left_held = 0;
		} break;
		case WM_CHAR:
		{
			Char character = (Char)(U1)wparam;
			if(character >= 32 && input_state.char_count < 32)
			{
				input_state.chars[input_state.char_count++] = character;
			}
		} break;
		case WM_KEYDOWN:
		{
			if (wparam == VK_BACK) input_state.backspace = 1;
			if (wparam == VK_RETURN) input_state.enter = 1;
			if (wparam == VK_ESCAPE) input_state.escape = 1;
		} break;
		case WM_SIZING:
		case WM_MOVING:
		{
			// Window is being resized or moved
			// modal loop is active, our main loop is blocked
			// Render a frame directly from here to keep the display live
			RECT client_rect;
			GetClientRect(window_handle, &client_rect);
			U4 window_width = (U4)(client_rect.right - client_rect.left);
			U4 window_height = (U4)(client_rect.bottom - client_rect.top);
			if (window_width == 0 || window_height == 0) break;

			renderer_begin_frame(g_renderer_context);
			run(&g_arena, &input_state, g_renderer_context, window_width, window_height);
			renderer_draw_frame(g_renderer_context, &g_arena, 1);
		} break;
		default:
		{
		} break;
	}
	return DefWindowProcA(window_handle, message_hash, wparam, lparam);
}

WindowInfo platform_get_window_info(Handle window_handle)
{
	RECT window_rect;
	GetClientRect(window_handle, &window_rect);

	U4 dpi = GetDpiForWindow(window_handle);
	float scale = (F4)dpi / 96.0f;

	WindowInfo result;
	result.width = (U4)((F4)(window_rect.right - window_rect.left) * scale);
	result.height = (U4)((F4)(window_rect.bottom - window_rect.top) * scale);
	return result;
}

U1 run_program(void)
{
#if FEATURE_DEBUG
	math_test();
#endif

	Instance instance = GetModuleHandleA(NULL);

	// Register window class
	WindowClass window_class = {
		.lpfnWndProc = window_procedure,
		.hInstance = instance,
		.lpszClassName = APPLICATION_NAME,
		//.style = CS_HREDRAW | CS_VREDRAW
	};
	
	RegisterClassA(&window_class);

	WindowHandle window_handle = CreateWindowExA(0, window_class.lpszClassName, APPLICATION_NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);
	DEBUG_RUNTIME_ASSERT(window_handle != NULL, "Failed to create window");

	g_arena = (Arena){0};
	memory_arena_create(&g_arena, 1024 * 1024); // 1 MB arena

#if RENDERER_VULKAN
	
#endif

	g_renderer_context = (RendererContext){0};
	if (!renderer_initialize(&g_renderer_context, &g_arena, window_handle, instance))
	{
		bebug_breakpoint();
		return 1;
	}

	// Main loop
	while (window_active)
	{
		//Reset input state for this frame
		input_state.mouse_left_pressed = 0;
		input_state.mouse_left_released = 0;
		input_state.char_count = 0;
		input_state.backspace = 0;
		input_state.enter = 0;
		input_state.escape = 0;

		MSG message;
		while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}

		RECT client_rect;
		GetClientRect(window_handle, &client_rect);
		U4 window_width = (U4)client_rect.right - (U4)client_rect.left;
		U4 window_height = (U4)client_rect.bottom - (U4)client_rect.top;

		if (window_width == 0 || window_height == 0) continue;

		renderer_begin_frame(g_renderer_context);

		run(
			&g_arena, 
			&input_state, 
			g_renderer_context, 
			window_width, window_height
		);

		renderer_draw_frame(g_renderer_context, &g_arena, 0);

		Sleep(1);
	}

	renderer_cleanup(g_renderer_context);

	//Startup((U4)argc, argv);
	return 0;
}

__attribute__((noreturn)) void entry_point(void)
{
	U1 result = run_program();
	ExitProcess(result);
}

void bebug_breakpoint(void)
{
	__debugbreak();
}

void platform_console_log(Char const * data, U8 size)
{
	Handle handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWord written;
	WriteFile(handle, data, (DWord)size, &written, NULL);
}

U4 platform_console_read(Char* data, U8 size)
{
	Handle handle = GetStdHandle(STD_INPUT_HANDLE);

	DWord bytes_read = 0;
	ReadFile(handle, data, (DWord)size, &bytes_read, NULL);

	return bytes_read;
}

U8 platform_console_log_file(Handle file_handle)
{
	Handle output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	U8 total_bytes_parsed = 0;

	const U4 buffer_size = 4096u;
	Char buffer[buffer_size];
	DWord bytes_read;
	DWord bytes_written;
	while (ReadFile(file_handle, buffer, buffer_size, &bytes_read, NULL) && bytes_read)
	{
		total_bytes_parsed += bytes_read;
		if (!WriteFile(output_handle, buffer, bytes_read, &bytes_written, NULL) || bytes_written != bytes_read)
		{
			bebug_breakpoint();
			CloseHandle(file_handle);
			return 0;
		}
	}

	return total_bytes_parsed;
}

FileReadHandle platform_file_open_read(const Char* path)
{
	FileReadHandle file_read_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_read_handle == INVALID_HANDLE_VALUE)
	{
		bebug_breakpoint();
		return NULL;
	}
	return file_read_handle;
}

void platform_file_close(FileReadHandle file_handle)
{
	CloseHandle(file_handle);
}

U1 platform_file_read(FileReadHandle file_handle, Char* buffer, U4 buffer_size, U4* bytes_read)
{
	if (ReadFile(file_handle, buffer, buffer_size, (LPDWORD)bytes_read, NULL))
	{
		return (*bytes_read) > 0;
	}

	bebug_breakpoint();
	return 0;
}

FileWriteHandle platform_file_open_write(const Char* path)
{
	FileWriteHandle file_write_handle = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_write_handle == INVALID_HANDLE_VALUE)
	{
		bebug_breakpoint();
		return NULL;
	}
	return file_write_handle;
}

U1 platform_file_write(FileWriteHandle file_handle, const Char* buffer, U4 buffer_size, U4* bytes_written)
{
	if (WriteFile(file_handle, buffer, buffer_size, (LPDWORD)bytes_written, NULL))
	{
		return (*bytes_written) > 0;
	}

	bebug_breakpoint();
	return 0;
}

U4 platform_console_peek(void)
{
	Handle handle = GetStdHandle(STD_INPUT_HANDLE);
	DWord bytes_available = 0;
	PeekNamedPipe(handle, NULL, 0, NULL, &bytes_available, NULL);
	return bytes_available;
}

void platform_console_clear(void)
{
	Handle handle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO buffer_info;

	if (!GetConsoleScreenBufferInfo(handle, &buffer_info))
	{
		// Note: Not buffering to console
		return;
	}

	DWord written;
	Coord home_position = { 0, 0 };
	DWord char_count = (DWord)(buffer_info.dwSize.X * buffer_info.dwSize.Y);

	FillConsoleOutputCharacterA(handle, ' ', char_count, home_position, &written);
	FillConsoleOutputAttribute(handle, buffer_info.wAttributes, char_count, home_position, &written);
	SetConsoleCursorPosition(handle, home_position);
}

void platform_reserve_pages(Binary** memory_ptr, U8 size)
{
	*memory_ptr = (Binary*)VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
	DEBUG_RUNTIME_ASSERT(*memory_ptr != NULL, "VirtualAlloc reserve failed");
}

void platform_commit_pages(Binary* memory_ptr, U8 size)
{
	LPVOID result = VirtualAlloc(memory_ptr, size, MEM_COMMIT, PAGE_READWRITE);
	DEBUG_RUNTIME_ASSERT(result, "VirtualAlloc commit failed");
}

void platform_window_create(const Char* title, U4 width, U4 height)
{
	HWND handle = CreateWindowExA(0L, "STATIC", title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, (S4)width, (S4)height, NULL, NULL, NULL, NULL);

	ShowWindow(handle, SW_SHOW);

	MSG	message;
	while (GetMessageA(&message, 0, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}

	//return 0;
}

U1 platform_window_process_messages(void)
{
	return 1;
}

void platform_window_destroy(void)
{

}

#endif
