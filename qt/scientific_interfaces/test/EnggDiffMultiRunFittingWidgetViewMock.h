#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetView
    : public MantidQt::CustomInterfaces::IEnggDiffMultiRunFittingWidgetView {

public:
  MOCK_METHOD0(getSelectedRunLabel, std::pair<int, size_t>());

  MOCK_METHOD1(plotFittedPeaks,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD1(plotFocusedRun,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD0(resetCanvas, void());

  MOCK_CONST_METHOD0(showFitResultsSelected, bool());

  MOCK_METHOD1(updateRunList,
               void(const std::vector<std::pair<int, size_t>> &runLabels));

  MOCK_METHOD2(userError, void(const std::string &errorTitle,
                               const std::string &errorDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
