#include "EnggDiffMultiRunFittingQtWidget.h"

#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingQtWidget::EnggDiffMultiRunFittingQtWidget(
    boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter,
    boost::shared_ptr<IEnggDiffractionUserMsg> messageProvider)
    : m_presenter(presenter), m_userMessageProvider(messageProvider) {
  setupUI();
  
  m_zoomTool = Mantid::Kernel::make_unique<QwtPlotZoomer>(
      QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner,
      QwtPicker::AlwaysOff, m_ui.plotArea->canvas());
  m_zoomTool->setRubberBandPen(QPen(Qt::black));
  m_zoomTool->setEnabled(false);
}

EnggDiffMultiRunFittingQtWidget::~EnggDiffMultiRunFittingQtWidget() {
  cleanUpPlot();
}

void EnggDiffMultiRunFittingQtWidget::cleanUpPlot() {
  for (auto &curve : m_focusedRunCurves) {
    curve->detach();
  }
  m_focusedRunCurves.clear();
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
    const std::vector<boost::shared_ptr<QwtData>> &curves) {
  for (size_t i = 0; i < curves.size(); ++i) {
    auto *curve = curves[i].get();
    auto plotCurve = Mantid::Kernel::make_unique<QwtPlotCurve>();

    plotCurve->setData(*curve);
    plotCurve->attach(m_ui.plotArea);
    m_focusedRunCurves.push_back(std::move(plotCurve));
  }
  m_ui.plotArea->replot();
  m_zoomTool->setZoomBase();
  m_zoomTool->setEnabled(true);
}

void EnggDiffMultiRunFittingQtWidget::processSelectRun() {
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::SelectRun);
}

void EnggDiffMultiRunFittingQtWidget::resetCanvas() {
  cleanUpPlot();
  m_ui.plotArea->replot();
  resetPlotZoomLevel();
}

void EnggDiffMultiRunFittingQtWidget::resetPlotZoomLevel() {
  m_ui.plotArea->setAxisAutoScale(QwtPlot::xBottom);
  m_ui.plotArea->setAxisAutoScale(QwtPlot::yLeft);
  m_zoomTool->setZoomBase(true);
}

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
