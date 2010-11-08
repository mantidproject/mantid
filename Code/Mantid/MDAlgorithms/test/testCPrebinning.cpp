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
#include <cxxtest/ParenPrinter.h>

int main() {
 return CxxTest::ParenPrinter().run();
}
#include "c:/Mantid/Code/Mantid/MDAlgorithms/test/testCPrebinning.h"

static testCPRebinning suite_testCPRebinning;

static CxxTest::List Tests_testCPRebinning = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testCPRebinning( "c:/Mantid/Code/Mantid/MDAlgorithms/test/testCPrebinning.h", 17, "testCPRebinning", suite_testCPRebinning, Tests_testCPRebinning );

static class TestDescription_testCPRebinning_testRebinInit : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPRebinning_testRebinInit() : CxxTest::RealTestDescription( Tests_testCPRebinning, suiteDescription_testCPRebinning, 21, "testRebinInit" ) {}
 void runTest() { suite_testCPRebinning.testRebinInit(); }
} testDescription_testCPRebinning_testRebinInit;

#include <cxxtest/Root.cpp>
