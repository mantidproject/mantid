// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_MOCKPROGRESSABLEVIEW_H
#define MANTID_MANTIDWIDGETS_MOCKPROGRESSABLEVIEW_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;

class MockProgressableView : public ProgressableView {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD0(clearProgress, void());
  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_CONST_METHOD0(isPercentageIndicator, bool());
  MOCK_METHOD0(setAsPercentageIndicator, void());
  MOCK_METHOD0(setAsEndlessIndicator, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif /* MANTID_MANTIDWIDGETS_MOCKPROGRESSABLEVIEW_H */
