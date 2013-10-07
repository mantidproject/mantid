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

bool suite_InvertMDDimTest_init = false;
#include "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/InvertMDDimTest.h"

static InvertMDDimTest suite_InvertMDDimTest;

static CxxTest::List Tests_InvertMDDimTest = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_InvertMDDimTest( "/home/scratch/mantdiwork/mantid/Code/Mantid/Framework/SINQ/test/InvertMDDimTest.h", 21, "InvertMDDimTest", suite_InvertMDDimTest, Tests_InvertMDDimTest );

static class TestDescription_suite_InvertMDDimTest_testName : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_InvertMDDimTest_testName() : CxxTest::RealTestDescription( Tests_InvertMDDimTest, suiteDescription_InvertMDDimTest, 24, "testName" ) {}
 void runTest() { suite_InvertMDDimTest.testName(); }
} testDescription_suite_InvertMDDimTest_testName;

static class TestDescription_suite_InvertMDDimTest_testInit : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_InvertMDDimTest_testInit() : CxxTest::RealTestDescription( Tests_InvertMDDimTest, suiteDescription_InvertMDDimTest, 29, "testInit" ) {}
 void runTest() { suite_InvertMDDimTest.testInit(); }
} testDescription_suite_InvertMDDimTest_testInit;

static class TestDescription_suite_InvertMDDimTest_testInversion : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_InvertMDDimTest_testInversion() : CxxTest::RealTestDescription( Tests_InvertMDDimTest, suiteDescription_InvertMDDimTest, 35, "testInversion" ) {}
 void runTest() { suite_InvertMDDimTest.testInversion(); }
} testDescription_suite_InvertMDDimTest_testInversion;

static class TestDescription_suite_InvertMDDimTest_testMetaDataCopy : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_InvertMDDimTest_testMetaDataCopy() : CxxTest::RealTestDescription( Tests_InvertMDDimTest, suiteDescription_InvertMDDimTest, 78, "testMetaDataCopy" ) {}
 void runTest() { suite_InvertMDDimTest.testMetaDataCopy(); }
} testDescription_suite_InvertMDDimTest_testMetaDataCopy;

