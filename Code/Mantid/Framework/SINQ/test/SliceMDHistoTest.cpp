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

bool suite_SliceMDHistoTest_init = false;
#include "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/SliceMDHistoTest.h"

static SliceMDHistoTest suite_SliceMDHistoTest;

static CxxTest::List Tests_SliceMDHistoTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_SliceMDHistoTest( "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/SliceMDHistoTest.h", 21, "SliceMDHistoTest", suite_SliceMDHistoTest, Tests_SliceMDHistoTest );

static class TestDescription_suite_SliceMDHistoTest_testName : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SliceMDHistoTest_testName() : CxxTest::RealTestDescription( Tests_SliceMDHistoTest, suiteDescription_SliceMDHistoTest, 24, "testName" ) {}
 void runTest() { suite_SliceMDHistoTest.testName(); }
} testDescription_suite_SliceMDHistoTest_testName;

static class TestDescription_suite_SliceMDHistoTest_testInit : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SliceMDHistoTest_testInit() : CxxTest::RealTestDescription( Tests_SliceMDHistoTest, suiteDescription_SliceMDHistoTest, 29, "testInit" ) {}
 void runTest() { suite_SliceMDHistoTest.testInit(); }
} testDescription_suite_SliceMDHistoTest_testInit;

static class TestDescription_suite_SliceMDHistoTest_testExec : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SliceMDHistoTest_testExec() : CxxTest::RealTestDescription( Tests_SliceMDHistoTest, suiteDescription_SliceMDHistoTest, 35, "testExec" ) {}
 void runTest() { suite_SliceMDHistoTest.testExec(); }
} testDescription_suite_SliceMDHistoTest_testExec;

