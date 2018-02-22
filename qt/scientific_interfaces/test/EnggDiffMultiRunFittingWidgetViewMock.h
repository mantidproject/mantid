#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetView
    : public IEnggDiffMultiRunFittingWidgetView {

public:
  MOCK_CONST_METHOD0(getSelectedRunLabel, RunLabel());

  MOCK_METHOD1(plotFittedPeaks,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD1(plotFocusedRun,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD1(reportPlotInvalidFittedPeaks, void(const RunLabel &runLabel));

  MOCK_METHOD1(reportPlotInvalidFocusedRun, void(const RunLabel &runLabel));

  MOCK_METHOD0(resetCanvas, void());

  MOCK_CONST_METHOD0(showFitResultsSelected, bool());

  MOCK_METHOD1(updateRunList, void(const std::vector<RunLabel> &runLabels));

  MOCK_METHOD2(userError, void(const std::string &errorTitle,
                               const std::string &errorDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
