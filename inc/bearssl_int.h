#ifndef __BEARSSL_INT_H__
#define __BEARSSL_INT_H__

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 199901L
#define BEARSSL_IS_C99
#endif
#endif

#ifdef BEARSSL_IS_C99
#include <stdint.h>

typedef uint8_t br_ssl_u8;
typedef uint16_t br_ssl_u16;
typedef uint32_t br_ssl_u32;
typedef uint64_t br_ssl_u64;

typedef int8_t br_ssl_i8;
typedef int16_t br_ssl_i16;
typedef int32_t br_ssl_i32;
typedef int64_t br_ssl_i64;
#else
#ifdef _MSC_VER
typedef unsigned __int8 br_ssl_u8;
typedef unsigned __int16 br_ssl_u16;
typedef unsigned __int32 br_ssl_u32;
typedef unsigned __int64 br_ssl_u64;

typedef __int8 br_ssl_i8;
typedef __int16 br_ssl_i16;
typedef __int32 br_ssl_i32;
typedef __int64 br_ssl_i64;
#else
typedef unsigned char br_ssl_u8;
typedef unsigned short br_ssl_u16;
typedef unsigned int br_ssl_u32;
typedef unsigned long long br_ssl_u64;

typedef signed char br_ssl_i8;
typedef signed short br_ssl_i16;
typedef signed int br_ssl_i32;
typedef signed long long br_ssl_i64;
#endif
#endif

#endif
