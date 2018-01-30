#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"
#include "ui_EnggDiffMultiRunFittingQtWidget.h"

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingQtWidget
    : public QWidget,
      public IEnggDiffMultiRunFittingWidgetView {
  Q_OBJECT

public:
  EnggDiffMultiRunFittingQtWidget(
      boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter);

  void updateRunList(const std::vector<std::pair<int, size_t>> &runLabels) override;

private:
  boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> m_presenter;

  Ui::EnggDiffMultiRunFittingWidget m_ui;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_
