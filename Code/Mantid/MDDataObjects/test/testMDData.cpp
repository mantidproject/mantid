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

#include "c:/Mantid/Code/Mantid/MDDataObjects/test/testMDData.h"

static testDND suite_testDND;

static CxxTest::List Tests_testDND = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testDND( "c:/Mantid/Code/Mantid/MDDataObjects/test/testMDData.h", 11, "testDND", suite_testDND, Tests_testDND );

static class TestDescription_testDND_testDNDRead : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testDNDRead() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 14, "testDNDRead" ) {}
 void runTest() { suite_testDND.testDNDRead(); }
} testDescription_testDND_testDNDRead;

