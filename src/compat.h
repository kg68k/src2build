// compat.h
// Copyright (C) 2025 TcbnErik
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
