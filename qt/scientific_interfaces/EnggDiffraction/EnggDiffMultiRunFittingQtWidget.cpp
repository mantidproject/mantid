#include "EnggDiffMultiRunFittingQtWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingQtWidget::EnggDiffMultiRunFittingQtWidget(
    boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter)
    : m_presenter(presenter) {
  m_ui.setupUi(this);
}

void EnggDiffMultiRunFittingQtWidget::updateRunList(
    const std::vector<std::pair<int, size_t>> &runLabels) {
  m_ui.listWidget_runLabels->clear();
  for (const auto &runLabel : runLabels) {
    const auto labelStr = QString::number(runLabel.first) + tr("_") +
                          QString::number(runLabel.second);
    m_ui.listWidget_runLabels->addItem(labelStr);
  }
}

} // CustomInterfaces
} // MantidQt
