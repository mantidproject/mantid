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
CxxTest::StaticSuiteDescription suiteDescription_testDND( "c:/Mantid/Code/Mantid/MDDataObjects/test/testMDData.h", 20, "testDND", suite_testDND, Tests_testDND );

static class TestDescription_testDND_testMDDataContstructor : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testMDDataContstructor() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 27, "testMDDataContstructor" ) {}
 void runTest() { suite_testDND.testMDDataContstructor(); }
} testDescription_testDND_testMDDataContstructor;

static class TestDescription_testDND_testDNDPrivateRead : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testDNDPrivateRead() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 39, "testDNDPrivateRead" ) {}
 void runTest() { suite_testDND.testDNDPrivateRead(); }
} testDescription_testDND_testDNDPrivateRead;

static class TestDescription_testDND_testDNDGet2DData : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testDNDGet2DData() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 46, "testDNDGet2DData" ) {}
 void runTest() { suite_testDND.testDNDGet2DData(); }
} testDescription_testDND_testDNDGet2DData;

static class TestDescription_testDND_testGet3DData : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testGet3DData() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 57, "testGet3DData" ) {}
 void runTest() { suite_testDND.testGet3DData(); }
} testDescription_testDND_testGet3DData;

static class TestDescription_testDND_testGet1Ddata : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testGet1Ddata() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 64, "testGet1Ddata" ) {}
 void runTest() { suite_testDND.testGet1Ddata(); }
} testDescription_testDND_testGet1Ddata;

static class TestDescription_testDND_testGet2Ddata : public CxxTest::RealTestDescription {
public:
 TestDescription_testDND_testGet2Ddata() : CxxTest::RealTestDescription( Tests_testDND, suiteDescription_testDND, 70, "testGet2Ddata" ) {}
 void runTest() { suite_testDND.testGet2Ddata(); }
} testDescription_testDND_testGet2Ddata;

