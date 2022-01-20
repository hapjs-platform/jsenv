/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "test_help.h"

#include <string>
#include <thread>

#include "hybrid-log.h"

namespace hybrid {

// define the test interface
#define DEFINE_EXPECT_EXPR(name, EXPECT)                                       \
  static bool expect_##name(const JSValue* value1, const JSValue* value2,      \
                            const char* message) {                             \
    switch (value1->Type()) {                                                  \
      case JSValue::kInt:                                                      \
        EXPECT(value1->IntVal(), value2->IntVal()) << message;                 \
        break;                                                                 \
      case JSValue::kUint:                                                     \
        EXPECT(value1->UintVal(), value2->UintVal()) << message;               \
        break;                                                                 \
      case JSValue::kFloat:                                                    \
        EXPECT(value1->FloatVal(), value2->FloatVal()) << message;             \
        break;                                                                 \
      case JSValue::kBoolean:                                                  \
        EXPECT(value1->Boolean(), value2->Boolean()) << message;               \
        break;                                                                 \
      case JSValue::kUTF8String:                                               \
        EXPECT(std::string(value1->UTF8Str()), std::string(value2->UTF8Str())) \
            << message;                                                        \
        break;                                                                 \
      default:                                                                 \
        return false;                                                          \
    }                                                                          \
    return true;                                                               \
  }

DEFINE_EXPECT_EXPR(eq, EXPECT_PRIMARY_EQ)
DEFINE_EXPECT_EXPR(ne, EXPECT_NE)
DEFINE_EXPECT_EXPR(le, EXPECT_LE)
DEFINE_EXPECT_EXPR(lt, EXPECT_LT)
DEFINE_EXPECT_EXPR(ge, EXPECT_GE)
DEFINE_EXPECT_EXPR(gt, EXPECT_GT)

#define DEFINE_TEST(name)                                                    \
  static bool test_##name(JSEnv* env, void* user_data, JSObject self,        \
                          const JSValue* argv, int argc, JSValue* presult) { \
    if (argc < 3) {                                                          \
      return false;                                                          \
    }                                                                        \
                                                                             \
    if (!argv[2].IsUTF8String()) {                                           \
      return false;                                                          \
    }                                                                        \
                                                                             \
    if (argv[0].Type() != argv[1].Type()) {                                  \
      return false;                                                          \
    }                                                                        \
                                                                             \
    return expect_##name(&argv[0], &argv[1], argv[2].UTF8Str());             \
  }

DEFINE_TEST(eq)
DEFINE_TEST(ne)
DEFINE_TEST(le)
DEFINE_TEST(lt)
DEFINE_TEST(ge)
DEFINE_TEST(gt)

#define DEFINE_FUNCTION(name) \
  { #name, test_##name, nullptr, JSEnv::kFlagUseUTF8 }

static JSFunctionDefinition test_functions[] = {DEFINE_FUNCTION(eq),
                                                DEFINE_FUNCTION(ne),
                                                DEFINE_FUNCTION(le),
                                                DEFINE_FUNCTION(lt),
                                                DEFINE_FUNCTION(ge),
                                                DEFINE_FUNCTION(gt),
                                                {0}};

static JSClassDefinition test_definition{"test",
                                         {0, nullptr, nullptr, 0},
                                         nullptr,
                                         nullptr,
                                         test_functions};

bool InitTestModule(JSEnv* jsenv) {
  JSClass clazz = jsenv->CreateClass(&test_definition, nullptr);

  JSObject object = jsenv->NewInstance(clazz);

  jsenv->SetGlobalValue("gtest", object);

  return true;
}

///////////////////////////////////////////////
// hybrid
class JSGTestCase : public ::testing::Test {
 public:
  explicit JSGTestCase(const TestInfo* ptest_info) : ptest_info_(ptest_info) {}

  void TestBody() override {
    g_jsenv->PushScope();

    JSValue result;

    g_jsenv->ExecuteScript(ptest_info_->code, ptest_info_->code_size, &result,
                           ptest_info_->file_name, ptest_info_->line_no);

    if (g_jsenv->HasException()) {
      JSException exception = g_jsenv->GetException();

      ALOGE("JSENV", "Test Error: %s.%s:%s(%d)\n%s",
            ptest_info_->test_case_name, ptest_info_->test_name,
            ptest_info_->file_name, static_cast<int>(ptest_info_->line_no),
            ptest_info_->code);

      ALOGE("JSENV", "Error (%d): %s", exception.type, exception.message);

      char szbuf[1024];
      snprintf(szbuf, sizeof(szbuf) - 1, "%s.%s:%s(%d) with error(%d): %s",
               ptest_info_->test_case_name, ptest_info_->test_name,
               ptest_info_->file_name, static_cast<int>(ptest_info_->line_no),
               exception.type, exception.message);

      EXPECT_EQ(true, false) << "execute " << szbuf;
    }

    g_jsenv->PopScope();
  }

 private:
  const TestInfo* ptest_info_;
};

class JSTestFactory : public ::testing::internal::TestFactoryBase {
 public:
  explicit JSTestFactory(const TestInfo* ptest_info) {
    ptest_info_ = ptest_info;
  }

  ::testing::Test* CreateTest() override {
    return new JSGTestCase(ptest_info_);
  }

 private:
  const TestInfo* ptest_info_;
};

::testing::TestInfo* MakeAndRegisterJSTestInfo(const TestInfo* ptest_info) {
  return ::testing::internal::MakeAndRegisterTestInfo(
      ptest_info->test_case_name, ptest_info->test_name, nullptr, nullptr,
      ::testing::internal::CodeLocation(ptest_info->file_name,
                                        ptest_info->line_no),
      ::testing::internal::GetTestTypeId(), ::testing::Test::SetUpTestCase,
      ::testing::Test::TearDownTestCase, new JSTestFactory(ptest_info));
}

}  // namespace hybrid
