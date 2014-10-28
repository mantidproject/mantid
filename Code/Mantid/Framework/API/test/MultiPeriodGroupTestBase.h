/*
 * MultiPeriodGroupTestBase.h
 *
 *  Created on: 18 Aug 2014
 *      Author: spu92482
 */

#ifndef MANTID_API_MULTIPERIODGROUPTESTBASE_H_
#define MANTID_API_MULTIPERIODGROUPTESTBASE_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidTestHelpers/FakeObjects.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;

class MultiPeriodGroupTestBase
{

protected:

// Helper method to add multiperiod logs to make a workspacegroup look like a real multiperiod workspace group.
  void add_periods_logs(WorkspaceGroup_sptr ws)
  {
    int nperiods = static_cast<int>(ws->size());
    for (size_t i = 0; i < ws->size(); ++i)
    {
      MatrixWorkspace_sptr currentWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));
      PropertyWithValue<int>* nperiodsProp = new PropertyWithValue<int>("nperiods", nperiods);
      currentWS->mutableRun().addLogData(nperiodsProp);
      PropertyWithValue<int>* currentPeriodsProp = new PropertyWithValue<int>("current_period",
          static_cast<int>(i + 1));
      currentWS->mutableRun().addLogData(currentPeriodsProp);
    }
  }

  /// Helper to fabricate a workspace group consisting of equal sized matrixworkspaces.
  WorkspaceGroup_sptr create_good_multiperiod_workspace_group(const std::string name)
  {
    MatrixWorkspace_sptr a = MatrixWorkspace_sptr(new WorkspaceTester);
    MatrixWorkspace_sptr b = MatrixWorkspace_sptr(new WorkspaceTester);

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    //group->setName(name);
    group->addWorkspace(a);
    group->addWorkspace(b);
    add_periods_logs(group);

    AnalysisDataService::Instance().addOrReplace(name + "_1", a);
    AnalysisDataService::Instance().addOrReplace(name + "_2", b);
    AnalysisDataService::Instance().addOrReplace(name, group);
    return group;
  }

};

#endif /* MANTID_API_MULTIPERIODGROUPTESTBASE_H_ */
