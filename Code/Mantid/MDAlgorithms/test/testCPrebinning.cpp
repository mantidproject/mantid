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

#include "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/MDAlgorithms/test/testCPrebinning.h"

static testCPRebinning suite_testCPRebinning;

static CxxTest::List Tests_testCPRebinning = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testCPRebinning( "C:/Users/wkc26243/Documents/work/MANTID/Code/Mantid/MDAlgorithms/test/testCPrebinning.h", 43, "testCPRebinning", suite_testCPRebinning, Tests_testCPRebinning );

static class TestDescription_testCPRebinning_testRebinInit : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPRebinning_testRebinInit() : CxxTest::RealTestDescription( Tests_testCPRebinning, suiteDescription_testCPRebinning, 49, "testRebinInit" ) {}
 void runTest() { suite_testCPRebinning.testRebinInit(); }
} testDescription_testCPRebinning_testRebinInit;

static class TestDescription_testCPRebinning_testInitSlicingProperty : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPRebinning_testInitSlicingProperty() : CxxTest::RealTestDescription( Tests_testCPRebinning, suiteDescription_testCPRebinning, 65, "testInitSlicingProperty" ) {}
 void runTest() { suite_testCPRebinning.testInitSlicingProperty(); }
} testDescription_testCPRebinning_testInitSlicingProperty;

static class TestDescription_testCPRebinning_testCPRExec : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPRebinning_testCPRExec() : CxxTest::RealTestDescription( Tests_testCPRebinning, suiteDescription_testCPRebinning, 83, "testCPRExec" ) {}
 void runTest() { suite_testCPRebinning.testCPRExec(); }
} testDescription_testCPRebinning_testCPRExec;

static class TestDescription_testCPRebinning_testCPRExecAgain : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPRebinning_testCPRExecAgain() : CxxTest::RealTestDescription( Tests_testCPRebinning, suiteDescription_testCPRebinning, 87, "testCPRExecAgain" ) {}
 void runTest() { suite_testCPRebinning.testCPRExecAgain(); }
} testDescription_testCPRebinning_testCPRExecAgain;

static class TestDescription_testCPRebinning_testRebinningResults : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPRebinning_testRebinningResults() : CxxTest::RealTestDescription( Tests_testCPRebinning, suiteDescription_testCPRebinning, 95, "testRebinningResults" ) {}
 void runTest() { suite_testCPRebinning.testRebinningResults(); }
} testDescription_testCPRebinning_testRebinningResults;

