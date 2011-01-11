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

#include "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/Framework/MDAlgorithms/test/testCPrebinning.h"

static testCPrebinning suite_testCPrebinning;

static CxxTest::List Tests_testCPrebinning = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testCPrebinning( "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/Framework/MDAlgorithms/test/testCPrebinning.h", 43, "testCPrebinning", suite_testCPrebinning, Tests_testCPrebinning );

static class TestDescription_testCPrebinning_testRebinInit : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testRebinInit() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 49, "testRebinInit" ) {}
 void runTest() { suite_testCPrebinning.testRebinInit(); }
} testDescription_testCPrebinning_testRebinInit;

static class TestDescription_testCPrebinning_testInitSlicingProperty : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testInitSlicingProperty() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 65, "testInitSlicingProperty" ) {}
 void runTest() { suite_testCPrebinning.testInitSlicingProperty(); }
} testDescription_testCPrebinning_testInitSlicingProperty;

static class TestDescription_testCPrebinning_testCPRExec : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testCPRExec() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 74, "testCPRExec" ) {}
 void runTest() { suite_testCPrebinning.testCPRExec(); }
} testDescription_testCPrebinning_testCPRExec;

static class TestDescription_testCPrebinning_testMDIMGCorrectness : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testMDIMGCorrectness() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 78, "testMDIMGCorrectness" ) {}
 void runTest() { suite_testCPrebinning.testMDIMGCorrectness(); }
} testDescription_testCPrebinning_testMDIMGCorrectness;

static class TestDescription_testCPrebinning_testCPRExecAgain : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testCPRExecAgain() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 93, "testCPRExecAgain" ) {}
 void runTest() { suite_testCPrebinning.testCPRExecAgain(); }
} testDescription_testCPrebinning_testCPRExecAgain;

static class TestDescription_testCPrebinning_testRebinningResults : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testRebinningResults() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 111, "testRebinningResults" ) {}
 void runTest() { suite_testCPrebinning.testRebinningResults(); }
} testDescription_testCPrebinning_testRebinningResults;

