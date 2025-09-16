// compat.h
// Copyright (C) 2025 TcbnErik
//
// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.

#ifndef COMPAT_H
#define COMPAT_H

#if defined(__GNUC__)
#ifndef __CHAR_UNSIGNED__
#error \
    "Plain 'char' type must be unsigned. Please specify '-funsigned-char' option."
#endif
#elif defined(_MSC_VER)
#ifndef _CHAR_UNSIGNED
#error "Plain 'char' type must be unsigned. Please specify '/J' option."
#endif
#else
#error "This compiler is not supported."
#endif

#if __STDC_VERSION__ >= 202311L
#define C23
#endif

#ifdef C23
#define UNUSED [[maybe_unused]]
#elif defined(__GNUC__) && (__GNUC__ >= 3)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#ifdef C23
#define NORETURN [[noreturn]]
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif

#ifdef __GNUC__
#define GCC_NORETURN __attribute__((noreturn))
#else
#define GCC_NORETURN
#endif

#ifdef __GNUC__
#define GCC_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
#define GCC_PRINTF(a, b)
#endif

#if !defined(C23) && !defined(nullptr)
#define nullptr ((void*)0)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// _countof()
#define C(array) (sizeof(array) / sizeof(array[0]))

#if defined(__human68k__) && defined(__LIBC__)
#define closedir(d) (closedir(d), 0)
#endif

#ifdef _MSC_VER
#define mkdir(p, m) _mkdir(p)
#endif

#endif
