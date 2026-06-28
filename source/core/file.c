#include "file.h"

#include "platform.h"
#include "debug.h"

FileReadHandle file_open_for_read(const Char* path)
{
	FileReadHandle read_handle = platform_file_open_read(path);
	DEBUG_RUNTIME_ASSERT(read_handle, "Expected: file opened successfully");
	return read_handle;
}

FileWriteHandle file_open_for_write(const Char* path)
{
	FileWriteHandle write_handle = platform_file_open_write(path);
	DEBUG_RUNTIME_ASSERT(write_handle, "Expected: file opened successfully");
	return write_handle;
}

void file_close(FileReadHandle file_handle)
{
	platform_file_close(file_handle);
}

U1 file_read(FileReadHandle file_handle, Char* buffer, U4 buffer_size, U4* bytes_read)
{
	return platform_file_read(file_handle, buffer, buffer_size, bytes_read);
}

U1 file_write(FileWriteHandle write_handle, const Char* buffer, U4 buffer_size, U4* bytes_written)
{
	return platform_file_write(write_handle, buffer, buffer_size, bytes_written);
}
