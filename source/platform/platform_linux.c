#include "../core/types.h"

#if PLATFORM_LINUX

#include "Platform.h"

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h> // file

int main(int argc, char** argv)
{
	Startup(argc, argv);
	return 0;
}

void bebug_breakpoint()
{
	__builtin_trap();
}

void ExitProcess(U8 exit_code)
{
    /*__asm__ volatile (
        "mov $60, %%rax\n"
        "mov %0, %%rdi\n"
        "syscall\n"
        :
        : "r"(exit_code)
        : "%rax", "%rdi"
    );*/
    exit(exit_code);
}

void platform_console_log(const Char* data, U4 size)
{
	write( STDOUT_FILENO, data, size);
}

U4 platform_console_read(const Char* data, U4 size)
{
    U4 bytes_read = read(STDIN_FILENO, (void *)data, size);
	return bytes_read;
}

void platform_console_clear()
{
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

FileReadHandle platform_file_open_read(const Char* path)
{
	S4 file_read_handle = open(path, O_RDONLY);
	if (file_read_handle < 0)
	{
		bebug_breakpoint();
		return NULL;
	}
	return (void*)(U8)file_read_handle;
}

void platform_file_close(FileReadHandle file_handle)
{
	close((S4)(U8)file_handle);
}

U1 platform_file_read(FileReadHandle file_handle, Char* data, U4 buffer_size, U4* bytes_read)
{
    *bytes_read = read((S4)(U8)file_handle, data, buffer_size);
	if (*bytes_read > 0)
	{
		return *bytes_read;
	}

	bebug_breakpoint();
	return 0;
}

FileWriteHandle platform_file_open_write(const Char* path)
{
	S4 file_write_handle = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (file_write_handle < 0)
	{
		bebug_breakpoint();
		return NULL;
	}
	return (void *)(U8)file_write_handle;
}

U1 platform_file_write(FileWriteHandle file_handle, const Char* data, U4 buffer_size, U4* bytes_written)
{
    *bytes_written = write((S4)(U8)file_handle, data, buffer_size);
	if(*bytes_written > 0)
    {
		return *bytes_written;
	}

	bebug_breakpoint();
	return 0;
}


#include <sys/mman.h>
void platform_reserve_pages(Binary** memory_ptr, U8 size)
{
	*memory_ptr = (Binary*)mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (*memory_ptr == MAP_FAILED)
	{
		bebug_breakpoint();
		ExitProcess(1);
	}
}
void platform_commit_pages(Binary* memory_ptr, U8 size)
{
	if (mprotect(memory_ptr, size, PROT_READ | PROT_WRITE) != 0)
	{
		bebug_breakpoint();
		ExitProcess(1);
	}
}

#endif
