/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_TEST_HELP_H_
#define HYBRID_TEST_HELP_H_

#include "JSEnv.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace hybrid {

struct TestInfo {
  const char* test_case_name;
  const char* test_name;
  const char* file_name;
  const char* code;
  size_t code_size;
  size_t line_no;
};

static inline ::testing::AssertionResult PrimaryValueCompare(
    const char* lhs_expr,
    const char* rhs_expr,
    double lhs,
    double rhs) {
  return ::testing::internal::DoubleNearPredFormat(lhs_expr, rhs_expr, ">0.001",
                                                   lhs, rhs, 0.001);
}

template <typename T1, typename T2>
static inline ::testing::AssertionResult PrimaryValueCompare(
    const char* lhs_expr,
    const char* rhs_expr,
    const T1& lhs,
    const T2& rhs) {
  return ::testing::internal::EqHelper<false>::Compare(lhs_expr, rhs_expr, lhs,
                                                       rhs);
}

#define EXPECT_PRIMARY_EQ(val1, val2) \
  EXPECT_PRED_FORMAT2(hybrid::PrimaryValueCompare, val1, val2)

bool InitTestModule(JSEnv* jsenv);

::testing::TestInfo* MakeAndRegisterJSTestInfo(const TestInfo* ptest_info);

extern JSEnv* g_jsenv;

}  // namespace hybrid

#endif  // HYBRID_TEST_HELP_H_
