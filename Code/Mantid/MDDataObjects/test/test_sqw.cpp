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
#include "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/MDDataObjects/test/test_sqw.h"

static tmain suite_tmain;

static CxxTest::List Tests_tmain = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_tmain( "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/MDDataObjects/test/test_sqw.h", 8, "tmain", suite_tmain, Tests_tmain );

static class TestDescription_tmain_testTMain : public CxxTest::RealTestDescription {
public:
 TestDescription_tmain_testTMain() : CxxTest::RealTestDescription( Tests_tmain, suiteDescription_tmain, 11, "testTMain" ) {}
 void runTest() { suite_tmain.testTMain(); }
} testDescription_tmain_testTMain;

#include <cxxtest/Root.cpp>
