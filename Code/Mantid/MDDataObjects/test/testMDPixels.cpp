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

#include "C:/Mantid/Code/Mantid/MDDataObjects/test/testMDPixels.h"

static testMDPixels suite_testMDPixels;

static CxxTest::List Tests_testMDPixels = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testMDPixels( "C:/Mantid/Code/Mantid/MDDataObjects/test/testMDPixels.h", 12, "testMDPixels", suite_testMDPixels, Tests_testMDPixels );

static class TestDescription_testMDPixels_testSQWConstructor : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDPixels_testSQWConstructor() : CxxTest::RealTestDescription( Tests_testMDPixels, suiteDescription_testMDPixels, 18, "testSQWConstructor" ) {}
 void runTest() { suite_testMDPixels.testSQWConstructor(); }
} testDescription_testMDPixels_testSQWConstructor;

static class TestDescription_testMDPixels_testSQWDNDread : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDPixels_testSQWDNDread() : CxxTest::RealTestDescription( Tests_testMDPixels, suiteDescription_testMDPixels, 21, "testSQWDNDread" ) {}
 void runTest() { suite_testMDPixels.testSQWDNDread(); }
} testDescription_testMDPixels_testSQWDNDread;

static class TestDescription_testMDPixels_testSQWnPix : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDPixels_testSQWnPix() : CxxTest::RealTestDescription( Tests_testMDPixels, suiteDescription_testMDPixels, 31, "testSQWnPix" ) {}
 void runTest() { suite_testMDPixels.testSQWnPix(); }
} testDescription_testMDPixels_testSQWnPix;

static class TestDescription_testMDPixels_testSQWreadEmptySelection : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDPixels_testSQWreadEmptySelection() : CxxTest::RealTestDescription( Tests_testMDPixels, suiteDescription_testMDPixels, 41, "testSQWreadEmptySelection" ) {}
 void runTest() { suite_testMDPixels.testSQWreadEmptySelection(); }
} testDescription_testMDPixels_testSQWreadEmptySelection;

static class TestDescription_testMDPixels_testSQWreadDataSelection : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDPixels_testSQWreadDataSelection() : CxxTest::RealTestDescription( Tests_testMDPixels, suiteDescription_testMDPixels, 52, "testSQWreadDataSelection" ) {}
 void runTest() { suite_testMDPixels.testSQWreadDataSelection(); }
} testDescription_testMDPixels_testSQWreadDataSelection;

