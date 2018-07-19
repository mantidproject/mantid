#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetModel
    : public IEnggDiffMultiRunFittingWidgetModel {

public:
  MOCK_METHOD2(addFittedPeaks,
               void(const RunLabel &runLabel,
                    const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD2(addFocusedRun, void(const RunLabel &runLabel,
                                   const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_CONST_METHOD0(getAllWorkspaceLabels, std::vector<RunLabel>());

  MOCK_CONST_METHOD1(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getFocusedRun,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(hasFittedPeaksForRun, bool(const RunLabel &runLabel));

  MOCK_METHOD1(removeRun, void(const RunLabel &runLabel));
};

DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_
