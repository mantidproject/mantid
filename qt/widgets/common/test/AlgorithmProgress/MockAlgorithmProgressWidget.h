#ifndef MANTID_MANTIDWIDGETS_MOCKALGORITHMPROGRESSWIDGET_H
#define MANTID_MANTIDWIDGETS_MOCKALGORITHMPROGRESSWIDGET_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressWidget.h"

#include <boost/enable_shared_from_this.hpp>
#include <gmock/gmock.h>
#include <memory>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace testing;
using namespace MantidQt::MantidWidgets;
class QProgressBar;
class QWidget;

class MockAlgorithmProgressWidget : public IAlgorithmProgressWidget {
public:
  MockAlgorithmProgressWidget() {
    m_presenter = std::make_shared<AlgorithmProgressPresenter>(
        static_cast<QWidget *>(nullptr), this);
  }
  virtual ~MockAlgorithmProgressWidget() {}

  MOCK_METHOD0(algorithmStarted, void());
  MOCK_METHOD0(algorithmEnded, void());
  MOCK_METHOD2(updateProgress, void(double, const QString &));
  MOCK_METHOD0(showDetailsDialog, void());

  std::shared_ptr<AlgorithmProgressPresenter> m_presenter;
};
#endif // MANTID_MANTIDWIDGETS_MOCKALGORITHMPROGRESSWIDGET_H
