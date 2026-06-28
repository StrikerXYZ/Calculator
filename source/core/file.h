#pragma once

#include "types.h"

typedef void* FileReadHandle;

typedef void* FileWriteHandle;

FileReadHandle file_open_for_read(const Char* path);
FileWriteHandle file_open_for_write(const Char* path);
void file_close(FileReadHandle file_handle);

U1 file_read(FileReadHandle file_handle, Char* buffer, U4 buffer_size, U4* bytes_read);
U1 file_write(FileReadHandle file_handle, const Char* buffer, U4 buffer_size, U4* bytes_written);
