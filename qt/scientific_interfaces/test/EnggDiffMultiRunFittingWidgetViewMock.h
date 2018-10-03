#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffMultiRunFittingWidgetView
    : public IEnggDiffMultiRunFittingWidgetView {

public:
  MOCK_CONST_METHOD0(getAllRunLabels, std::vector<RunLabel>());

  MOCK_CONST_METHOD0(getSelectedRunLabel, boost::optional<RunLabel>());

  MOCK_METHOD1(plotFittedPeaks,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD1(plotFocusedRun,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD2(plotToSeparateWindow,
               void(const std::string &focusedRunName,
                    const boost::optional<std::string> fittedPeaksName));

  MOCK_METHOD0(reportNoRunSelectedForPlot, void());

  MOCK_METHOD1(reportPlotInvalidFittedPeaks, void(const RunLabel &runLabel));

  MOCK_METHOD1(reportPlotInvalidFocusedRun, void(const RunLabel &runLabel));

  MOCK_METHOD0(resetCanvas, void());

  MOCK_METHOD1(setEnabled, void(const bool));

  MOCK_METHOD1(
      setMessageProvider,
      void(boost::shared_ptr<IEnggDiffractionUserMsg> messageProvider));

  MOCK_METHOD1(setPresenter,
               void(boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter>
                        presenter));

  MOCK_CONST_METHOD0(showFitResultsSelected, bool());

  MOCK_METHOD1(updateRunList, void(const std::vector<RunLabel> &runLabels));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEWMOCK_H_
