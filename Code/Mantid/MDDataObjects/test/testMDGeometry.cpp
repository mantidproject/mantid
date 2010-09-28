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

#include "testMDGeometry.h"

static testGeometry suite_testGeometry;

static CxxTest::List Tests_testGeometry = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testGeometry( "testMDGeometry.h", 18, "testGeometry", suite_testGeometry, Tests_testGeometry );

static class TestDescription_testGeometry_testGeometryC : public CxxTest::RealTestDescription {
public:
 TestDescription_testGeometry_testGeometryC() : CxxTest::RealTestDescription( Tests_testGeometry, suiteDescription_testGeometry, 21, "testGeometryC" ) {}
 void runTest() { suite_testGeometry.testGeometryC(); }
} testDescription_testGeometry_testGeometryC;

