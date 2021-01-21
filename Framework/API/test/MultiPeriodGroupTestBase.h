// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * MultiPeriodGroupTestBase.h
 *
 *  Created on: 18 Aug 2014
 *      Author: spu92482
 */

#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class MultiPeriodGroupTestBase {

protected:
  // Helper method to add multiperiod logs to make a workspacegroup look like a
  // real multiperiod workspace group.
  void add_periods_logs(const WorkspaceGroup_sptr &ws) {
    int nperiods = static_cast<int>(ws->size());
    for (size_t i = 0; i < ws->size(); ++i) {
      MatrixWorkspace_sptr currentWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));
      PropertyWithValue<int> *nperiodsProp = new PropertyWithValue<int>("nperiods", nperiods);
      currentWS->mutableRun().addLogData(nperiodsProp);
      PropertyWithValue<int> *currentPeriodsProp =
          new PropertyWithValue<int>("current_period", static_cast<int>(i + 1));
      currentWS->mutableRun().addLogData(currentPeriodsProp);
    }
  }

  /// Helper to fabricate a workspace group consisting of equal sized
  /// matrixworkspaces.
  WorkspaceGroup_sptr create_good_multiperiod_workspace_group(const std::string &name) {
    MatrixWorkspace_sptr a = MatrixWorkspace_sptr(new WorkspaceTester);
    MatrixWorkspace_sptr b = MatrixWorkspace_sptr(new WorkspaceTester);

    WorkspaceGroup_sptr group = std::make_shared<WorkspaceGroup>();
    // group->setName(name);
    group->addWorkspace(a);
    group->addWorkspace(b);
    add_periods_logs(group);

    AnalysisDataService::Instance().addOrReplace(name + "_1", a);
    AnalysisDataService::Instance().addOrReplace(name + "_2", b);
    AnalysisDataService::Instance().addOrReplace(name, group);
    return group;
  }
};
