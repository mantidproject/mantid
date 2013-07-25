#ifndef RENAMEWORKSPACETEST_H_
#define RENAMEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/RenameWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class RenameWorkspaceTest: public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "RenameWorkspace");
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( alg.version(), 1);
  }

  void testInit()
  {
    Mantid::Algorithms::RenameWorkspace alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg2.isInitialized());

    const std::vector<Property*> props = alg2.getProperties();
    TS_ASSERT_EQUALS( props.size(), 2);

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace");
    TS_ASSERT( props[0]->isDefault());
    TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[0]));

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace");
    TS_ASSERT( props[1]->isDefault());
    TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[1]));

  }

  void testExec()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::RenameWorkspace alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS"));
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSRenamed"));

    TS_ASSERT_THROWS_NOTHING( alg3.execute());
    TS_ASSERT( alg3.isExecuted());

    Workspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>("WSRenamed"));
    TS_ASSERT( result);

    TS_ASSERT_THROWS_ANYTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>("InputWS"));

    AnalysisDataService::Instance().remove("WSRenamed");
  }

  void testSameNames()
  {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::RenameWorkspace alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS"));

    TS_ASSERT_THROWS( alg3.setPropertyValue("OutputWorkspace","InputWS"), std::invalid_argument);

    AnalysisDataService::Instance().remove("InputWS");
  }

  void testExists()
  {
    MatrixWorkspace_sptr ws1 = createWorkspace();
    AnalysisDataService::Instance().add("ws1", ws1);

    MatrixWorkspace_sptr ws2 = createWorkspace();
    AnalysisDataService::Instance().add("ws2", ws2);

    Mantid::Algorithms::RenameWorkspace algInstance;
    algInstance.initialize();

    TS_ASSERT_THROWS_NOTHING(algInstance.setPropertyValue("InputWorkspace","ws1"));

    TS_ASSERT_THROWS(algInstance.setPropertyValue("OutputWorkspace","ws2"), std::invalid_argument);

    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void testGroup()
  {
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();

    WorkspaceGroup_sptr group(new WorkspaceGroup);
    ads.add("oldName",group);
    MatrixWorkspace_sptr member1 = createWorkspace();
    ads.add("oldName_1",member1);
    group->add("oldName_1");
    MatrixWorkspace_sptr member2 = createWorkspace();
    ads.add("oldName_2",member2);
    group->add("oldName_2");

    Mantid::Algorithms::RenameWorkspace renamer;
    renamer.initialize();
    TS_ASSERT_THROWS_NOTHING( renamer.setPropertyValue("InputWorkspace", "oldName") )
    TS_ASSERT_THROWS_NOTHING( renamer.setPropertyValue("OutputWorkspace", "newName") );
    TS_ASSERT( renamer.execute() )

    Workspace_sptr result;
    TS_ASSERT_THROWS_NOTHING ( result = ads.retrieve("newName") )
    WorkspaceGroup_sptr resultGroup;
    TS_ASSERT ( resultGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(result) )
    // It should actually be the same workspace as the input
    TS_ASSERT( resultGroup == group )
    // The output group should have the same workspaces in, with new names of course
    TS_ASSERT_EQUALS( resultGroup->size(), 2 )
    TS_ASSERT_EQUALS( resultGroup->getItem(0), member1 )
    TS_ASSERT_EQUALS( resultGroup->getItem(0)->name(), "newName_1" )
    TS_ASSERT_EQUALS( resultGroup->getItem(1), member2 )
    TS_ASSERT_EQUALS( resultGroup->getItem(1)->name(), "newName_2" )
    // The old ones should not be in the ADS
    TS_ASSERT_THROWS( ads.retrieve("oldName"), Mantid::Kernel::Exception::NotFoundError )
    TS_ASSERT_THROWS( ads.retrieve("oldName_1"), Mantid::Kernel::Exception::NotFoundError )
    TS_ASSERT_THROWS( ads.retrieve("oldName_2"), Mantid::Kernel::Exception::NotFoundError )
    // The new ones should be
    TS_ASSERT_THROWS_NOTHING ( ads.retrieve("newName_1") )
    TS_ASSERT_THROWS_NOTHING ( ads.retrieve("newName_1") )

    // Clean up
    ads.clear();
  }

  MatrixWorkspace_sptr createWorkspace()
  {
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(4, 4, 0.5);
    return inputWS;
  }

private:
  Mantid::Algorithms::RenameWorkspace alg;

};

#endif /*RENAMEWORKSPACETEST_H_*/
