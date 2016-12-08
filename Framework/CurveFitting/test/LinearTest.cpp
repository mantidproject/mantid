/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#define _CXXTEST_HAVE_STD
#define _CXXTEST_HAVE_EH
#define _CXXTEST_LONGLONG long long
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>
#include <cxxtest/TestMain.h>

bool suite_LinearTest_init = false;
#include "/home/cs/reimund/mantid/Framework/CurveFitting/test/Functions/LinearTest.h"

static LinearTest *suite_LinearTest = 0;

static CxxTest::List Tests_LinearTest = {0, 0};
CxxTest::DynamicSuiteDescription<LinearTest>
    suiteDescription_LinearTest("/home/cs/reimund/mantid/Framework/"
                                "CurveFitting/test/Functions/LinearTest.h",
                                12, "LinearTest", Tests_LinearTest,
                                suite_LinearTest, 16, 19);

static class TestDescription_suite_LinearTest_test_category final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_LinearTest_test_category()
      : CxxTest::RealTestDescription(Tests_LinearTest,
                                     suiteDescription_LinearTest, 21,
                                     "test_category") {}
  void runTest() override final {
    if (suite_LinearTest)
      suite_LinearTest->test_category();
  }
} testDescription_suite_LinearTest_test_category;

static class TestDescription_suite_LinearTest_test_calculate final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_LinearTest_test_calculate()
      : CxxTest::RealTestDescription(Tests_LinearTest,
                                     suiteDescription_LinearTest, 33,
                                     "test_calculate") {}
  void runTest() override final {
    if (suite_LinearTest)
      suite_LinearTest->test_calculate();
  }
} testDescription_suite_LinearTest_test_calculate;
