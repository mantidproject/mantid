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

bool suite_ProjectMDTest_init = false;
#include "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/ProjectMDTest.h"

static ProjectMDTest suite_ProjectMDTest;

static CxxTest::List Tests_ProjectMDTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_ProjectMDTest( "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/ProjectMDTest.h", 21, "ProjectMDTest", suite_ProjectMDTest, Tests_ProjectMDTest );

static class TestDescription_suite_ProjectMDTest_testName : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testName() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 24, "testName" ) {}
 void runTest() { suite_ProjectMDTest.testName(); }
} testDescription_suite_ProjectMDTest_testName;

static class TestDescription_suite_ProjectMDTest_testInit : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testInit() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 29, "testInit" ) {}
 void runTest() { suite_ProjectMDTest.testInit(); }
} testDescription_suite_ProjectMDTest_testInit;

static class TestDescription_suite_ProjectMDTest_testProjectZ : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testProjectZ() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 35, "testProjectZ" ) {}
 void runTest() { suite_ProjectMDTest.testProjectZ(); }
} testDescription_suite_ProjectMDTest_testProjectZ;

static class TestDescription_suite_ProjectMDTest_testProjectHalfZ : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testProjectHalfZ() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 76, "testProjectHalfZ" ) {}
 void runTest() { suite_ProjectMDTest.testProjectHalfZ(); }
} testDescription_suite_ProjectMDTest_testProjectHalfZ;

static class TestDescription_suite_ProjectMDTest_testProjectX : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testProjectX() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 117, "testProjectX" ) {}
 void runTest() { suite_ProjectMDTest.testProjectX(); }
} testDescription_suite_ProjectMDTest_testProjectX;

static class TestDescription_suite_ProjectMDTest_testProjectY : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testProjectY() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 158, "testProjectY" ) {}
 void runTest() { suite_ProjectMDTest.testProjectY(); }
} testDescription_suite_ProjectMDTest_testProjectY;

static class TestDescription_suite_ProjectMDTest_testMetaDataCopy : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_ProjectMDTest_testMetaDataCopy() : CxxTest::RealTestDescription( Tests_ProjectMDTest, suiteDescription_ProjectMDTest, 199, "testMetaDataCopy" ) {}
 void runTest() { suite_ProjectMDTest.testMetaDataCopy(); }
} testDescription_suite_ProjectMDTest_testMetaDataCopy;

