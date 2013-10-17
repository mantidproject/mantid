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

bool suite_LoadFlexiNexusTest_init = false;
#include "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/LoadFlexiNexusTest.h"

static LoadFlexiNexusTest suite_LoadFlexiNexusTest;

static CxxTest::List Tests_LoadFlexiNexusTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_LoadFlexiNexusTest( "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/LoadFlexiNexusTest.h", 19, "LoadFlexiNexusTest", suite_LoadFlexiNexusTest, Tests_LoadFlexiNexusTest );

static class TestDescription_suite_LoadFlexiNexusTest_testName : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_LoadFlexiNexusTest_testName() : CxxTest::RealTestDescription( Tests_LoadFlexiNexusTest, suiteDescription_LoadFlexiNexusTest, 23, "testName" ) {}
 void runTest() { suite_LoadFlexiNexusTest.testName(); }
} testDescription_suite_LoadFlexiNexusTest_testName;

static class TestDescription_suite_LoadFlexiNexusTest_testInit : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_LoadFlexiNexusTest_testInit() : CxxTest::RealTestDescription( Tests_LoadFlexiNexusTest, suiteDescription_LoadFlexiNexusTest, 28, "testInit" ) {}
 void runTest() { suite_LoadFlexiNexusTest.testInit(); }
} testDescription_suite_LoadFlexiNexusTest_testInit;

static class TestDescription_suite_LoadFlexiNexusTest_testExec3D : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_LoadFlexiNexusTest_testExec3D() : CxxTest::RealTestDescription( Tests_LoadFlexiNexusTest, suiteDescription_LoadFlexiNexusTest, 34, "testExec3D" ) {}
 void runTest() { suite_LoadFlexiNexusTest.testExec3D(); }
} testDescription_suite_LoadFlexiNexusTest_testExec3D;

static class TestDescription_suite_LoadFlexiNexusTest_testExec1D : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_LoadFlexiNexusTest_testExec1D() : CxxTest::RealTestDescription( Tests_LoadFlexiNexusTest, suiteDescription_LoadFlexiNexusTest, 87, "testExec1D" ) {}
 void runTest() { suite_LoadFlexiNexusTest.testExec1D(); }
} testDescription_suite_LoadFlexiNexusTest_testExec1D;

