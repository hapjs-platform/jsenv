// Copyright (c) 2021, the hapjs-platform Project Contributors
// SPDX-License-Identifier: EPL-1.0

#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#include <stddef.h>  // For size_t.

#define DISALLOW_COPY(TypeName) \
    TypeName(const TypeName&) = delete

#define DISALLOW_ASSIGN(TypeName) \
    TypeName& operator=(const TypeName&) = delete

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    DISALLOW_COPY(TypeName); \
    DISALLOW_ASSIGN(TypeName)

template <typename T, size_t N> char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#define UNUSED __attribute__((unused))

#endif  // BASE_MACROS_H_
