// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTER_H_
#include "../../../ISISReflectometry/GUI/Common/Plotter.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockRunsTablePresenter : public IRunsTablePresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IRunsPresenter *));
  MOCK_CONST_METHOD0(runsTable, RunsTable const &());
  MOCK_METHOD0(mutableRunsTable, RunsTable &());
  MOCK_METHOD0(notifyRowStateChanged, void());
  MOCK_METHOD0(notifyRowOutputsChanged, void());
  MOCK_METHOD0(notifyRemoveAllRowsAndGroupsRequested, void());
  MOCK_METHOD1(mergeAdditionalJobs, void(ReductionJobs const &));
  MOCK_METHOD0(reductionPaused, void());
  MOCK_METHOD0(reductionResumed, void());
  MOCK_METHOD0(autoreductionPaused, void());
  MOCK_METHOD0(autoreductionResumed, void());
  MOCK_METHOD1(instrumentChanged, void(std::string const &));
  MOCK_METHOD0(settingsChanged, void());
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSTABLEPRESENTER_H_
