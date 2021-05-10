// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/GUI/Common/Plotter.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MockRunsTablePresenter : public IRunsTablePresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IRunsPresenter *));
  MOCK_CONST_METHOD0(runsTable, RunsTable const &());
  MOCK_METHOD0(mutableRunsTable, RunsTable &());
  MOCK_METHOD0(notifyRowStateChanged, void());
  MOCK_METHOD1(notifyRowStateChanged, void(boost::optional<Item const &>));
  MOCK_METHOD0(notifyRowOutputsChanged, void());
  MOCK_METHOD1(notifyRowOutputsChanged, void(boost::optional<Item const &>));
  MOCK_METHOD0(notifyRemoveAllRowsAndGroupsRequested, void());
  MOCK_METHOD1(mergeAdditionalJobs, void(ReductionJobs const &));
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchReductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchReductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionResumed, void());
  MOCK_METHOD1(notifyInstrumentChanged, void(std::string const &));
  MOCK_METHOD0(settingsChanged, void());
  MOCK_METHOD1(setTablePrecision, void(int &));
  MOCK_METHOD0(resetTablePrecision, void());
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE