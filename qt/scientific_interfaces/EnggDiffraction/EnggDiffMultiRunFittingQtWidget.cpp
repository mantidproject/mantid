#include "EnggDiffMultiRunFittingQtWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingQtWidget::EnggDiffMultiRunFittingQtWidget(
    boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter)
    : m_presenter(presenter) {
  setupUI();
}

std::pair<int, size_t> EnggDiffMultiRunFittingQtWidget::getSelectedRunLabel() {
  const auto currentLabel = m_ui.listWidget_runLabels->currentItem()->text();
  const auto pieces = currentLabel.split("_");
  if (pieces.size() != 2) {
    throw std::runtime_error(
        "Unexpected run label: \"" + currentLabel.toStdString() +
        "\". Please contact the development team with this message");
  }
  return std::make_pair(pieces[0].toInt(), pieces[1].toUInt());
}

void EnggDiffMultiRunFittingQtWidget::processSelectRun() {
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::SelectRun);
}

void EnggDiffMultiRunFittingQtWidget::setupUI() {
  m_ui.setupUi(this);

  connect(m_ui.listWidget_runLabels, SIGNAL(itemSelectionChanged()), this,
          SLOT(processSelectRun()));
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
