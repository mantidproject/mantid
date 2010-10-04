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

#include "c:/Mantid/Code/Mantid/MDDataObjects/test/testWorkspaceGeometry.h"

static testWorkspaceGm suite_testWorkspaceGm;

static CxxTest::List Tests_testWorkspaceGm = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_testWorkspaceGm( "c:/Mantid/Code/Mantid/MDDataObjects/test/testWorkspaceGeometry.h", 19, "testWorkspaceGm", suite_testWorkspaceGm, Tests_testWorkspaceGm );

static class TestDescription_testWorkspaceGm_testWorkspaceGeometry : public CxxTest::RealTestDescription {
public:
 TestDescription_testWorkspaceGm_testWorkspaceGeometry() : CxxTest::RealTestDescription( Tests_testWorkspaceGm, suiteDescription_testWorkspaceGm, 22, "testWorkspaceGeometry" ) {}
 void runTest() { suite_testWorkspaceGm.testWorkspaceGeometry(); }
} testDescription_testWorkspaceGm_testWorkspaceGeometry;

