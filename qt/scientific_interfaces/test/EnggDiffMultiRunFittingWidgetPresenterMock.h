// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetAdderFake.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetPresenter
    : public IEnggDiffMultiRunFittingWidgetPresenter {
public:
  MOCK_METHOD2(addFittedPeaks,
               void(const RunLabel &runLabel,
                    const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD1(addFocusedRun, void(const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_CONST_METHOD0(getAllRunLabels, std::vector<RunLabel>());

  MOCK_CONST_METHOD1(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getFocusedRun,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD0(getSelectedRunLabel, boost::optional<RunLabel>());

  // Must be faked so that we can return a unique_ptr out of it
  std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
  getWidgetAdder() const override;

  MOCK_CONST_METHOD0(hasSelectedRunLabel, bool());

  MOCK_METHOD1(
      notify,
      void(IEnggDiffMultiRunFittingWidgetPresenter::Notification notif));
};

std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
MockEnggDiffMultiRunFittingWidgetPresenter::getWidgetAdder() const {
  return std::make_unique<FakeEnggDiffMultiRunFittingWidgetAdder>();
}

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERMOCK_H_
