/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#define _CXXTEST_HAVE_STD
#define _CXXTEST_HAVE_EH
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>

#include "testMDDimension.h"

static testMDDimension suite_testMDDimension;

static CxxTest::List Tests_testMDDimension = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testMDDimension( "testMDDimension.h", 48, "testMDDimension", suite_testMDDimension, Tests_testMDDimension );

static class TestDescription_testMDDimension_testDimensionPars : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDDimension_testDimensionPars() : CxxTest::RealTestDescription( Tests_testMDDimension, suiteDescription_testMDDimension, 51, "testDimensionPars" ) {}
 void runTest() { suite_testMDDimension.testDimensionPars(); }
} testDescription_testMDDimension_testDimensionPars;

static class TestDescription_testMDDimension_testDimensionRes : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDDimension_testDimensionRes() : CxxTest::RealTestDescription( Tests_testMDDimension, suiteDescription_testMDDimension, 108, "testDimensionRes" ) {}
 void runTest() { suite_testMDDimension.testDimensionRes(); }
} testDescription_testMDDimension_testDimensionRes;

