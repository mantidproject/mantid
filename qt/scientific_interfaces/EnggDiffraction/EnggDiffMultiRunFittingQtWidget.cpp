#include "EnggDiffMultiRunFittingQtWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingQtWidget::EnggDiffMultiRunFittingQtWidget(
    boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter,
    boost::shared_ptr<IEnggDiffractionUserMsg> messageProvider)
    : m_presenter(presenter), m_userMessageProvider(messageProvider) {
  setupUI();
}

std::pair<int, size_t>
EnggDiffMultiRunFittingQtWidget::getSelectedRunLabel() const {
  const auto currentLabel = m_ui.listWidget_runLabels->currentItem()->text();
  const auto pieces = currentLabel.split("_");
  if (pieces.size() != 2) {
    throw std::runtime_error(
        "Unexpected run label: \"" + currentLabel.toStdString() +
        "\". Please contact the development team with this message");
  }
  return std::make_pair(pieces[0].toInt(), pieces[1].toUInt());
}

void EnggDiffMultiRunFittingQtWidget::plotFittedPeaks(
    const std::vector<boost::shared_ptr<QwtData>> &curve) {
  UNUSED_ARG(curve);
  throw std::runtime_error("plotFittedPeaks not yet implemented");
}

void EnggDiffMultiRunFittingQtWidget::plotFocusedRun(
    const std::vector<boost::shared_ptr<QwtData>> &curve) {}

void EnggDiffMultiRunFittingQtWidget::processSelectRun() {
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::SelectRun);
}

void EnggDiffMultiRunFittingQtWidget::resetCanvas() {}

void EnggDiffMultiRunFittingQtWidget::setupUI() {
  m_ui.setupUi(this);

  connect(m_ui.listWidget_runLabels, SIGNAL(itemSelectionChanged()), this,
          SLOT(processSelectRun()));
}

bool EnggDiffMultiRunFittingQtWidget::showFitResultsSelected() const {
  return m_ui.checkBox_plotFittedPeaks->isChecked();
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

void EnggDiffMultiRunFittingQtWidget::userError(
    const std::string &errorTitle, const std::string &errorDescription) {
  m_userMessageProvider->userError(errorTitle, errorDescription);
}

} // CustomInterfaces
} // MantidQt
