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

#include "C:/Mantid/Code/Mantid/MDDataObjects/test/testMDGeometry.h"

static testMDGeometry suite_testMDGeometry;

static CxxTest::List Tests_testMDGeometry = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testMDGeometry( "C:/Mantid/Code/Mantid/MDDataObjects/test/testMDGeometry.h", 18, "testMDGeometry", suite_testMDGeometry, Tests_testMDGeometry );

static class TestDescription_testMDGeometry_testGeometryConstr : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDGeometry_testGeometryConstr() : CxxTest::RealTestDescription( Tests_testMDGeometry, suiteDescription_testMDGeometry, 23, "testGeometryConstr" ) {}
 void runTest() { suite_testMDGeometry.testGeometryConstr(); }
} testDescription_testMDGeometry_testGeometryConstr;

static class TestDescription_testMDGeometry_testMDGeometryDimAccessors : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDGeometry_testMDGeometryDimAccessors() : CxxTest::RealTestDescription( Tests_testMDGeometry, suiteDescription_testMDGeometry, 27, "testMDGeometryDimAccessors" ) {}
 void runTest() { suite_testMDGeometry.testMDGeometryDimAccessors(); }
} testDescription_testMDGeometry_testMDGeometryDimAccessors;

static class TestDescription_testMDGeometry_testMDGeomIntegrated : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDGeometry_testMDGeomIntegrated() : CxxTest::RealTestDescription( Tests_testMDGeometry, suiteDescription_testMDGeometry, 34, "testMDGeomIntegrated" ) {}
 void runTest() { suite_testMDGeometry.testMDGeomIntegrated(); }
} testDescription_testMDGeometry_testMDGeomIntegrated;

static class TestDescription_testMDGeometry_testMDGeomDimAcessors : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDGeometry_testMDGeomDimAcessors() : CxxTest::RealTestDescription( Tests_testMDGeometry, suiteDescription_testMDGeometry, 40, "testMDGeomDimAcessors" ) {}
 void runTest() { suite_testMDGeometry.testMDGeomDimAcessors(); }
} testDescription_testMDGeometry_testMDGeomDimAcessors;

static class TestDescription_testMDGeometry_testSlicingProperty : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDGeometry_testSlicingProperty() : CxxTest::RealTestDescription( Tests_testMDGeometry, suiteDescription_testMDGeometry, 57, "testSlicingProperty" ) {}
 void runTest() { suite_testMDGeometry.testSlicingProperty(); }
} testDescription_testMDGeometry_testSlicingProperty;

static class TestDescription_testMDGeometry_testMDGeomSetFromSlice : public CxxTest::RealTestDescription {
public:
 TestDescription_testMDGeometry_testMDGeomSetFromSlice() : CxxTest::RealTestDescription( Tests_testMDGeometry, suiteDescription_testMDGeometry, 72, "testMDGeomSetFromSlice" ) {}
 void runTest() { suite_testMDGeometry.testMDGeomSetFromSlice(); }
} testDescription_testMDGeometry_testMDGeomSetFromSlice;

