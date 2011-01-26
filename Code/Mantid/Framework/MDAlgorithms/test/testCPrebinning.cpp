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

#include "c:/Mantid/Code/Mantid/Framework/MDAlgorithms/test/testCPrebinning.h"

static testCPrebinning suite_testCPrebinning;

static CxxTest::List Tests_testCPrebinning = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testCPrebinning( "c:/Mantid/Code/Mantid/Framework/MDAlgorithms/test/testCPrebinning.h", 43, "testCPrebinning", suite_testCPrebinning, Tests_testCPrebinning );

static class TestDescription_testCPrebinning_testRebinInit : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testRebinInit() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 50, "testRebinInit" ) {}
 void runTest() { suite_testCPrebinning.testRebinInit(); }
} testDescription_testCPrebinning_testRebinInit;

static class TestDescription_testCPrebinning_testGetSlicingProperty : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testGetSlicingProperty() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 68, "testGetSlicingProperty" ) {}
 void runTest() { suite_testCPrebinning.testGetSlicingProperty(); }
} testDescription_testCPrebinning_testGetSlicingProperty;

static class TestDescription_testCPrebinning_testCPRExec : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testCPRExec() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 75, "testCPRExec" ) {}
 void runTest() { suite_testCPrebinning.testCPRExec(); }
} testDescription_testCPrebinning_testCPRExec;

static class TestDescription_testCPrebinning_testRebinnedWSExists : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testRebinnedWSExists() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 79, "testRebinnedWSExists" ) {}
 void runTest() { suite_testCPrebinning.testRebinnedWSExists(); }
} testDescription_testCPrebinning_testRebinnedWSExists;

static class TestDescription_testCPrebinning_testEqRebinCorrectness : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testEqRebinCorrectness() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 87, "testEqRebinCorrectness" ) {}
 void runTest() { suite_testCPrebinning.testEqRebinCorrectness(); }
} testDescription_testCPrebinning_testEqRebinCorrectness;

static class TestDescription_testCPrebinning_testCPRRebinAgainSmaller : public CxxTest::RealTestDescription {
public:
 TestDescription_testCPrebinning_testCPRRebinAgainSmaller() : CxxTest::RealTestDescription( Tests_testCPrebinning, suiteDescription_testCPrebinning, 102, "testCPRRebinAgainSmaller" ) {}
 void runTest() { suite_testCPrebinning.testCPRRebinAgainSmaller(); }
} testDescription_testCPrebinning_testCPRRebinAgainSmaller;

