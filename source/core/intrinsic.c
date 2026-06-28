#include "intrinsic.h"

#ifdef _MSC_VER
#include <intrin.h>
#endif // MSVC

// Compiler check
#if !defined(__clang__) || !defined(_MSC_VER)
#error "Expected clang-cl"
#endif

// C standard check
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201710L)
#error "At least C17 required (C23 expected)"
#endif

// Floating point sanity
#if defined(__FAST_MATH__)
#error "Fast math enabled (FMA likely allowed)"
#endif


U8 intrinsic_rdtsc(void)
{
#ifdef _MSC_VER
	return __rdtsc();
#elif defined(__clang__) || defined(__GNUC__)
	return __builtin_ia32_rdtsc();
#else
	#error "Unsupported compiler"
#endif // MSVC
}
