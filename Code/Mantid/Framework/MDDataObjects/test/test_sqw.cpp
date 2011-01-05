/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>
#include <cxxtest/ParenPrinter.h>

int main() {
 return CxxTest::ParenPrinter().run();
}
#include "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/Framework/MDDataObjects/test/test_sqw.h"

static testSQW suite_testSQW;

static CxxTest::List Tests_testSQW = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testSQW( "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/Framework/MDDataObjects/test/test_sqw.h", 8, "testSQW", suite_testSQW, Tests_testSQW );

static class TestDescription_testSQW_testTMain : public CxxTest::RealTestDescription {
public:
 TestDescription_testSQW_testTMain() : CxxTest::RealTestDescription( Tests_testSQW, suiteDescription_testSQW, 11, "testTMain" ) {}
 void runTest() { suite_testSQW.testTMain(); }
} testDescription_testSQW_testTMain;

#include <cxxtest/Root.cpp>
