#ifndef MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWMOCKOBJECT_H
#define MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWMOCKOBJECT_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;

class MockProgressableView : public ProgressableView {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD0(clearProgress, void());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

#endif /* MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWMOCKOBJECT_H */
