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

bool suite_SplineInterpolationTest_init = false;
#include "/home/cs/reimund/mantid/Framework/CurveFitting/test/Algorithms/SplineInterpolationTest.h"

static SplineInterpolationTest *suite_SplineInterpolationTest = 0;

static CxxTest::List Tests_SplineInterpolationTest = {0, 0};
CxxTest::DynamicSuiteDescription<SplineInterpolationTest>
    suiteDescription_SplineInterpolationTest(
        "/home/cs/reimund/mantid/Framework/CurveFitting/test/Algorithms/"
        "SplineInterpolationTest.h",
        13, "SplineInterpolationTest", Tests_SplineInterpolationTest,
        suite_SplineInterpolationTest, 17, 20);

static class TestDescription_suite_SplineInterpolationTest_test_Init final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_test_Init()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     27, "test_Init") {}
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->test_Init();
  }
} testDescription_suite_SplineInterpolationTest_test_Init;

static class TestDescription_suite_SplineInterpolationTest_testExec final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_testExec()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     33, "testExec") {}
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->testExec();
  }
} testDescription_suite_SplineInterpolationTest_testExec;

static class
    TestDescription_suite_SplineInterpolationTest_testInterpolationRange final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_testInterpolationRange()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     49, "testInterpolationRange") {}
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->testInterpolationRange();
  }
} testDescription_suite_SplineInterpolationTest_testInterpolationRange;

static class TestDescription_suite_SplineInterpolationTest_testExecHistogramData
    final : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_testExecHistogramData()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     65, "testExecHistogramData") {}
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->testExecHistogramData();
  }
} testDescription_suite_SplineInterpolationTest_testExecHistogramData;

static class
    TestDescription_suite_SplineInterpolationTest_testExecMultipleSpectra final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_testExecMultipleSpectra()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     81, "testExecMultipleSpectra") {}
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->testExecMultipleSpectra();
  }
} testDescription_suite_SplineInterpolationTest_testExecMultipleSpectra;

static class TestDescription_suite_SplineInterpolationTest_testAxisCopy final
    : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_testAxisCopy()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     97, "testAxisCopy") {}
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->testAxisCopy();
  }
} testDescription_suite_SplineInterpolationTest_testAxisCopy;

static class
    TestDescription_suite_SplineInterpolationTest_testOutOfOrderInterploationPoints
        final : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTest_testOutOfOrderInterploationPoints()
      : CxxTest::RealTestDescription(Tests_SplineInterpolationTest,
                                     suiteDescription_SplineInterpolationTest,
                                     130, "testOutOfOrderInterploationPoints") {
  }
  void runTest() override final {
    if (suite_SplineInterpolationTest)
      suite_SplineInterpolationTest->testOutOfOrderInterploationPoints();
  }
} testDescription_suite_SplineInterpolationTest_testOutOfOrderInterploationPoints;

static SplineInterpolationTestPerformance
    suite_SplineInterpolationTestPerformance;

static CxxTest::List Tests_SplineInterpolationTestPerformance = {0, 0};
CxxTest::StaticSuiteDescription
    suiteDescription_SplineInterpolationTestPerformance(
        "/home/cs/reimund/mantid/Framework/CurveFitting/test/Algorithms/"
        "SplineInterpolationTest.h",
        236, "SplineInterpolationTestPerformance",
        suite_SplineInterpolationTestPerformance,
        Tests_SplineInterpolationTestPerformance);

static class
    TestDescription_suite_SplineInterpolationTestPerformance_testSplineInterpolationPerformance
        final : public CxxTest::RealTestDescription {
public:
  TestDescription_suite_SplineInterpolationTestPerformance_testSplineInterpolationPerformance()
      : CxxTest::RealTestDescription(
            Tests_SplineInterpolationTestPerformance,
            suiteDescription_SplineInterpolationTestPerformance, 267,
            "testSplineInterpolationPerformance") {}
  void runTest() override final {
    suite_SplineInterpolationTestPerformance
        .testSplineInterpolationPerformance();
  }
} testDescription_suite_SplineInterpolationTestPerformance_testSplineInterpolationPerformance;
