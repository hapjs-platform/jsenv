// Copyright (c) 2021, the hapjs-platform Project Contributors
// SPDX-License-Identifier: EPL-1.0

#ifndef BASE_BASE_EXPORT_H_
#define BASE_BASE_EXPORT_H_

#if defined(BASE_IMPLEMENTATION)
#define BASE_EXPORT __attribute__((visibility("default")))
#else
#define BASE_EXPORT
#endif

#endif  // BASE_BASE_EXPORT_H_
