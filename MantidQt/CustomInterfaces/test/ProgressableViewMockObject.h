#ifndef MANTID_CUSTOMINTERFACES_PROGRESSABLEVIEWMOCKOBJECT_H
#define MANTID_CUSTOMINTERFACES_PROGRESSABLEVIEWMOCKOBJECT_H

#include "MantidQtCustomInterfaces/ProgressableView.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

class MockProgressableView : public ProgressableView {
public:
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD0(clearProgress, void());
  ~MockProgressableView() override {}
};

#endif /* MANTID_CUSTOMINTERFACES_PROGRESSABLEVIEWMOCKOBJECT_H */
