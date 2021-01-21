// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffMultiRunFittingWidgetView : public IEnggDiffMultiRunFittingWidgetView {

public:
  MOCK_CONST_METHOD0(getAllRunLabels, std::vector<RunLabel>());

  MOCK_CONST_METHOD0(getSelectedRunLabel, boost::optional<RunLabel>());

  MOCK_METHOD1(plotFittedPeaks, void(const std::vector<std::shared_ptr<QwtData>> &curve));

  MOCK_METHOD1(plotFocusedRun, void(const std::vector<std::shared_ptr<QwtData>> &curve));

  MOCK_METHOD2(plotToSeparateWindow,
               void(const std::string &focusedRunName, const boost::optional<std::string> fittedPeaksName));

  MOCK_METHOD0(reportNoRunSelectedForPlot, void());

  MOCK_METHOD1(reportPlotInvalidFittedPeaks, void(const RunLabel &runLabel));

  MOCK_METHOD1(reportPlotInvalidFocusedRun, void(const RunLabel &runLabel));

  MOCK_METHOD0(resetCanvas, void());

  MOCK_METHOD1(setEnabled, void(const bool));

  MOCK_METHOD1(setMessageProvider, void(std::shared_ptr<IEnggDiffractionUserMsg> messageProvider));

  MOCK_METHOD1(setPresenter, void(std::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter));

  MOCK_CONST_METHOD0(showFitResultsSelected, bool());

  MOCK_METHOD1(updateRunList, void(const std::vector<RunLabel> &runLabels));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
