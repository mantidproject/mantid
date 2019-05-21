// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTER_H_
#include "../../../ISISReflectometry/GUI/Plotting/Plotter.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockRunsTablePresenter : public RunsTablePresenter {
public:
  MockRunsTablePresenter(IRunsTableView *view,
                         std::vector<std::string> const &instruments,
                         double thetaTolerance, ReductionJobs reductionJobs,
                         const IPlotter &plotter)
      : RunsTablePresenter(view, instruments, thetaTolerance, reductionJobs,
                           plotter){};
  MOCK_METHOD0(notifyProcessRequested, void());
  MOCK_METHOD0(notifyPauseRequested, void());
  MOCK_METHOD0(notifyInsertRowRequested, void());
  MOCK_METHOD0(notifyInsertGroupRequested, void());
  MOCK_METHOD0(notifyDeleteRowRequested, void());
  MOCK_METHOD0(notifyDeleteGroupRequested, void());
  MOCK_METHOD1(notifyFilterChanged, void(std::string const &));
  MOCK_METHOD0(notifyExpandAllRequested, void());
  MOCK_METHOD0(notifyCollapseAllRequested, void());

  MOCK_METHOD4(notifyCellTextChanged,
               void(MantidQt::MantidWidgets::Batch::RowLocation const &, int,
                    std::string const &, std::string const &));
  MOCK_METHOD0(notifySelectionChanged, void());
  MOCK_METHOD1(notifyRowInserted,
               void(MantidQt::MantidWidgets::Batch::RowLocation const &));
  MOCK_METHOD1(
      notifyRemoveRowsRequested,
      void(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &));
  MOCK_METHOD0(notifyCutRowsRequested, void());
  MOCK_METHOD0(notifyCopyRowsRequested, void());
  MOCK_METHOD0(notifyPasteRowsRequested, void());
  MOCK_METHOD0(notifyFilterReset, void());

  MOCK_METHOD1(mergeAdditionalJobs, void(ReductionJobs const &));
  MOCK_CONST_METHOD0(reductionJobs, ReductionJobs const &());
  MOCK_METHOD0(reductionJobs, ReductionJobs &());
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSTABLEPRESENTER_H_
