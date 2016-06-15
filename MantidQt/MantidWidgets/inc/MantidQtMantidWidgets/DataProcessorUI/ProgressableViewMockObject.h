#ifndef MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWMOCKOBJECT_H
#define MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWMOCKOBJECT_H

#include "MantidQtMantidWidgets/ProgressableView.h"
#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;

class MockProgressableView : public ProgressableView {
public:
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD0(clearProgress, void());
  ~MockProgressableView() override {}
};

#endif /* MANTID_MANTIDWIDGETS_PROGRESSABLEVIEWMOCKOBJECT_H */
