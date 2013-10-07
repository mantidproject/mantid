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

bool suite_MDHistoToWorkspace2DTest_init = false;
#include "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/MDHistoToWorkspace2DTest.h"

static MDHistoToWorkspace2DTest suite_MDHistoToWorkspace2DTest;

static CxxTest::List Tests_MDHistoToWorkspace2DTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_MDHistoToWorkspace2DTest( "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/MDHistoToWorkspace2DTest.h", 20, "MDHistoToWorkspace2DTest", suite_MDHistoToWorkspace2DTest, Tests_MDHistoToWorkspace2DTest );

static class TestDescription_suite_MDHistoToWorkspace2DTest_testName : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_MDHistoToWorkspace2DTest_testName() : CxxTest::RealTestDescription( Tests_MDHistoToWorkspace2DTest, suiteDescription_MDHistoToWorkspace2DTest, 23, "testName" ) {}
 void runTest() { suite_MDHistoToWorkspace2DTest.testName(); }
} testDescription_suite_MDHistoToWorkspace2DTest_testName;

static class TestDescription_suite_MDHistoToWorkspace2DTest_testInit : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_MDHistoToWorkspace2DTest_testInit() : CxxTest::RealTestDescription( Tests_MDHistoToWorkspace2DTest, suiteDescription_MDHistoToWorkspace2DTest, 28, "testInit" ) {}
 void runTest() { suite_MDHistoToWorkspace2DTest.testInit(); }
} testDescription_suite_MDHistoToWorkspace2DTest_testInit;

static class TestDescription_suite_MDHistoToWorkspace2DTest_testExec : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_MDHistoToWorkspace2DTest_testExec() : CxxTest::RealTestDescription( Tests_MDHistoToWorkspace2DTest, suiteDescription_MDHistoToWorkspace2DTest, 34, "testExec" ) {}
 void runTest() { suite_MDHistoToWorkspace2DTest.testExec(); }
} testDescription_suite_MDHistoToWorkspace2DTest_testExec;

