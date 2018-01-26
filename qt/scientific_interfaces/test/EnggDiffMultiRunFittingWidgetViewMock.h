#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetView
    : public MantidQt::CustomInterfaces::IEnggDiffMultiRunFittingWidgetView {

public:
  MOCK_METHOD3(addFittedPeaks,
               void(const int runNumber, const size_t bank,
                    const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD3(addFocusedRun, void(const int runNumber, const size_t bank,
                                   const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_CONST_METHOD2(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD0(getFittedPeaksWorkspaceToAdd,
                     Mantid::API::MatrixWorkspace_sptr());

  MOCK_CONST_METHOD0(getFittedPeaksBankIDToAdd, size_t());

  MOCK_CONST_METHOD0(getFittedPeaksBankIDToReturn, size_t());

  MOCK_CONST_METHOD0(getFittedPeaksRunNumberToAdd, int());

  MOCK_CONST_METHOD0(getFittedPeaksRunNumberToReturn, int());

  MOCK_CONST_METHOD2(getFocusedRun,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD0(getFocusedWorkspaceToAdd,
                     Mantid::API::MatrixWorkspace_sptr());

  MOCK_CONST_METHOD0(getFocusedRunBankIDToAdd, size_t());

  MOCK_CONST_METHOD0(getFocusedRunNumberToAdd, int());

  MOCK_METHOD1(setFittedPeaksWorkspaceToReturn,
               void(const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD2(userError, void(const std::string &errorTitle,
                               const std::string &errorDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
