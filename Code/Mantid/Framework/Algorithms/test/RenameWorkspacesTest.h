#ifndef RENAMEWORKSPACESTEST_H_
#define RENAMEWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/RenameWorkspaces.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class RenameWorkspacesTest: public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "RenameWorkspaces");
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( alg.version(), 1);
  }

  void testInit()
  {
    Mantid::Algorithms::RenameWorkspaces alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg2.isInitialized());

    const std::vector<Property*> props = alg2.getProperties();
    TS_ASSERT_EQUALS( props.size(), 4);

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspaces");
    TS_ASSERT( props[0]->isDefault());
    //TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[0]));

    TS_ASSERT_EQUALS( props[1]->name(), "WorkspaceNames");
    TS_ASSERT( props[1]->isDefault());
    //TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[1]));

    TS_ASSERT_EQUALS( props[2]->name(), "Prefix");
    TS_ASSERT( props[2]->isDefault());
    // TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[1]));

  }

  void testExec()
  {
    MatrixWorkspace_sptr inputWS1 = createWorkspace();
    MatrixWorkspace_sptr inputWS2 = createWorkspace();
    AnalysisDataService::Instance().add("InputWS1", inputWS1);
    AnalysisDataService::Instance().add("InputWS2", inputWS2);

    Mantid::Algorithms::RenameWorkspaces alg3;
    alg3.setRethrows( true ); // Ensure exceptions are thrown to this test
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspaces","InputWS1, InputWS2"));
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("WorkspaceNames","NewName1, NewName2"));

    TS_ASSERT_THROWS_NOTHING( alg3.execute());
    //TS_ASSERT( alg3.isExecuted());

    //Workspace_sptr result;
    //TS_ASSERT_THROWS_NOTHING(
    //    result = AnalysisDataService::Instance().retrieveWS<Workspace>("WSRenamed"));
    //TS_ASSERT( result);

    //TS_ASSERT_THROWS_ANYTHING(
    //    result = AnalysisDataService::Instance().retrieveWS<Workspace>("InputWS"));

    AnalysisDataService::Instance().remove("InputWS1");
    AnalysisDataService::Instance().remove("InputWS2");
  }

  void testPrefix()
  {
    MatrixWorkspace_sptr inputWS1 = createWorkspace();
    MatrixWorkspace_sptr inputWS2 = createWorkspace();
    AnalysisDataService::Instance().add("InputWS1", inputWS1);
    AnalysisDataService::Instance().add("InputWS2", inputWS2);

    Mantid::Algorithms::RenameWorkspaces alg4;
    alg4.initialize();
    alg4.setPropertyValue("InputWorkspaces","InputWS1, InputWS2");
    alg4.setRethrows( true ); // Ensure exceptions are thrown to this test
    TS_ASSERT_THROWS_NOTHING( alg4.setPropertyValue("Prefix","A_"));

    TS_ASSERT_THROWS_NOTHING( alg4.execute());

    AnalysisDataService::Instance().remove("InputWS1");
    AnalysisDataService::Instance().remove("InputWS2");
  }

  void testSuffix()
  {
    MatrixWorkspace_sptr inputWS1 = createWorkspace();
    MatrixWorkspace_sptr inputWS2 = createWorkspace();
    AnalysisDataService::Instance().add("InputWS1", inputWS1);
    AnalysisDataService::Instance().add("InputWS2", inputWS2);

    Mantid::Algorithms::RenameWorkspaces alg5;
    alg5.initialize();
    alg5.setPropertyValue("InputWorkspaces","InputWS1, InputWS2");
    alg5.setRethrows( true ); // Ensure exceptions are thrown to this test
    TS_ASSERT_THROWS_NOTHING( alg5.setPropertyValue("Suffix","_1"));

    TS_ASSERT_THROWS_NOTHING( alg5.execute());

    AnalysisDataService::Instance().remove("InputWS1");
    AnalysisDataService::Instance().remove("InputWS2");
  }

  void testInvalidArguments()
  {
    MatrixWorkspace_sptr inputWS1 = createWorkspace();
    MatrixWorkspace_sptr inputWS2 = createWorkspace();
    AnalysisDataService::Instance().add("InputWS1", inputWS1);
    AnalysisDataService::Instance().add("InputWS2", inputWS2);
    Mantid::Algorithms::RenameWorkspaces alg6;
    alg6.setRethrows( true );  // Ensure it will throw any exceptions to this test
    alg6.initialize();
    alg6.setPropertyValue("InputWorkspaces","InputWS1, InputWS2");
    // Check throw if no workspace name set
    TS_ASSERT_THROWS(alg6.execute(),std::invalid_argument);
    // Check throw if conflicting workspace names are set
    alg6.setPropertyValue("WorkspaceNames","NewName1, NewName2");
    alg6.setPropertyValue("Prefix","A_");
    TS_ASSERT_THROWS(alg6.execute(),std::invalid_argument);
    alg6.setPropertyValue("Suffix","_1");
    TS_ASSERT_THROWS(alg6.execute(),std::invalid_argument);
    alg6.setPropertyValue("Prefix","");
    TS_ASSERT_THROWS(alg6.execute(),std::invalid_argument);
   
    AnalysisDataService::Instance().remove("InputWS1");
    AnalysisDataService::Instance().remove("InputWS2");

  }

  MatrixWorkspace_sptr createWorkspace()
  {
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(4, 4, 0.5);
    return inputWS;
  }

 
private:
  Mantid::Algorithms::RenameWorkspaces alg;

};

#endif /*RENAMEWORKSPACESTEST_H_*/
