#pragma once

#define APPLICATION_NAME "Spark"

#pragma clang diagnostic ignored "-Wunused-macros"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"

#include <stdint.h>

// Define static types
#define local static
#define global static //variables cannot be used in other translation units and initialized to 0 by default
#define internal static

typedef uint8_t U1;
typedef uint16_t U2;
typedef uint32_t U4;
typedef uint64_t U8;

typedef int8_t S1;
typedef int16_t S2;
typedef int32_t S4;
typedef int64_t S8;

typedef float F4;
typedef double F8;

typedef void Void;
typedef char Char;
typedef U1 Binary;

//Generic handle type
typedef void* Handle;

#define U1_MAX UINT8_MAX
#define U2_MAX UINT16_MAX
#define U4_MAX UINT32_MAX
#define U8_MAX UINT64_MAX
#define S1_MAX INT8_MAX
#define S2_MAX INT16_MAX
#define S4_MAX INT32_MAX
#define S8_MAX INT64_MAX

#define U1_MIN 0
#define U2_MIN 0
#define U4_MIN 0
#define U8_MIN 0
#define S1_MIN INT8_MIN
#define S2_MIN INT16_MIN
#define S4_MIN INT32_MIN
#define S8_MIN INT64_MIN

#define U1_MAX_STRING_SIZE sizeof("255") - 1
#define U2_MAX_STRING_SIZE sizeof("65535") - 1
#define U4_MAX_STRING_SIZE sizeof("4294967295") - 1
#define U8_MAX_STRING_SIZE sizeof("18446744073709551615") - 1
#define S1_MAX_STRING_SIZE sizeof("-128") - 1
#define S2_MAX_STRING_SIZE sizeof("-32768") - 1
#define S4_MAX_STRING_SIZE sizeof("-2147483648") - 1
#define S8_MAX_STRING_SIZE sizeof("-9223372036854775808") - 1

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#if defined(_WIN64) || defined(_WIN32)
#define PLATFORM_WINDOWS 1
#else
#define PLATFORM_WINDOWS 0
#endif

#if defined(__linux__)
#define PLATFORM_LINUX 1
#else
#define PLATFORM_LINUX 0
#endif

#ifdef NDEBUG
#define FEATURE_DEBUG 1
#else
#define FEATURE_DEBUG 1
#endif

#define RENDERER_VULKAN 1
