#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetAdderFake.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetPresenter
    : public IEnggDiffMultiRunFittingWidgetPresenter {
public:
  MOCK_METHOD3(addFittedPeaks,
               void(const int runNumber, const size_t bank,
                    const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD3(addFocusedRun, void(const int runNumber, const size_t bank,
                                   const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_CONST_METHOD2(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD2(getFocusedRun,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  // Must be faked so that we can return a unique_ptr out of it
  std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
  getWidgetAdder() const override;

  MOCK_METHOD1(
      notify,
      void(IEnggDiffMultiRunFittingWidgetPresenter::Notification notif));
};

std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
MockEnggDiffMultiRunFittingWidgetPresenter::getWidgetAdder() const {
  return std::make_unique<FakeEnggDiffMultiRunFittingWidgetAdder>();
}

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTERMOCK_H_
