#pragma once

#ifdef __cplusplus
#error "Expected: Should be running as C and not C++"
#endif

#include "platform.h"

#define DEBUG_RUNTIME_ASSERT(_condition_, _message_) \
	do { \
		if (!(_condition_)) { \
			bebug_breakpoint(); \
			platform_console_log (_message_, (U4)(sizeof(_message_) - 1)); \
		} \
	} while (0)

#define DEBUG_COMPILETIME_ASSERT(_condition_, _message_) \
	_Static_assert(_condition_, _message_)
	
