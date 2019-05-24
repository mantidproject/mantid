#ifndef MANTID_MANTIDWIDGETS_MOCKALGORITHMPROGRESSDIALOGWIDGET_H
#define MANTID_MANTIDWIDGETS_MOCKALGORITHMPROGRESSDIALOGWIDGET_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressDialogWidget.h"
#include "MockAlgorithmProgressWidget.h"

#include <boost/enable_shared_from_this.hpp>
#include <gmock/gmock.h>
#include <memory>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace testing;
using namespace MantidQt::MantidWidgets;
class QProgressBar;
class QWidget;

class MockAlgorithmProgressDialogWidget
    : public IAlgorithmProgressDialogWidget {
public:
  MockAlgorithmProgressDialogWidget() {
    mainProgressBar = std::make_shared<MockAlgorithmProgressWidget>();
    auto pres = mainProgressBar->m_presenter.get();
    m_presenter = std::make_shared<AlgorithmProgressDialogPresenter>(
        static_cast<QWidget *>(nullptr), this, pres->model());
  }
  virtual ~MockAlgorithmProgressDialogWidget() {}

  MOCK_METHOD1(addAlgorithm, std::pair<QTreeWidgetItem *, QProgressBar *>(
                                 Mantid::API::IAlgorithm_sptr));

  std::shared_ptr<AlgorithmProgressDialogPresenter> m_presenter;
  // This is the mocked main progress bar,
  // always shown on the Workbench GUI
  std::shared_ptr<MockAlgorithmProgressWidget> mainProgressBar;
};
#endif // MANTID_MANTIDWIDGETS_MOCKALGORITHMPROGRESSWIDGET_H
