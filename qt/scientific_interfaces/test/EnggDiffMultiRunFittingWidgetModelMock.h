#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetModel
    : public MantidQt::CustomInterfaces::IEnggDiffMultiRunFittingWidgetModel {

public:
  MOCK_METHOD3(addFittedPeaks,
               void(const int runNumber, const size_t bank,
                    const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD3(addFocusedRun, void(const int runNumber, const size_t bank,
                                   const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_CONST_METHOD0(getAllWorkspaceLabels,
                     std::vector<std::pair<int, size_t>>());

  MOCK_CONST_METHOD2(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD2(getFocusedRun,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_
