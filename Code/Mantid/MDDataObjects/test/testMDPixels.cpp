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

#include "c:/Mantid/Code/Mantid/MDDataObjects/test/testMDPixels.h"

static testMDPixels suite_testMDPixels;

static CxxTest::List Tests_testMDPixels = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testMDPixels( "c:/Mantid/Code/Mantid/MDDataObjects/test/testMDPixels.h", 11, "testMDPixels", suite_testMDPixels, Tests_testMDPixels );

static class TestDescription_testMDPixels_testSQW : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDPixels_testSQW() : CxxTest::RealTestDescription( Tests_testMDPixels, suiteDescription_testMDPixels, 14, "testSQW" ) {}
 void runTest() { suite_testMDPixels.testSQW(); }
} testDescription_testMDPixels_testSQW;

