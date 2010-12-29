#ifndef CONVERTTOMATRIXWORKSPACETEST_H_
#define CONVERTTOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/CheckWorkspacesMatch.h"

class ConvertToMatrixWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( cloner.name(), "ConvertToMatrixWorkspace" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( cloner.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( cloner.category(), "General" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( cloner.initialize() )
    TS_ASSERT( cloner.isInitialized() )
  }

  // TODO: Make this more appropriate for testing the copy of the histogram representation of
  // an event workspace.
  void testExec()
  {
    if ( !cloner.isInitialized() ) cloner.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace", "in");
    loader.execute();

    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("InputWorkspace","in") )
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("OutputWorkspace","out") )

    TS_ASSERT( cloner.execute() )

    // Best way to test this is to use the CheckWorkspacesMatch algorithm
    Mantid::Algorithms::CheckWorkspacesMatch checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1","in");
    checker.setPropertyValue("Workspace2","out");
    checker.execute();

    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Success!" )
  }

private:
  Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
};

#endif /*CONVERTTOMATRIXWORKSPACETEST_H_*/
