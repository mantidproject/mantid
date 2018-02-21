#include "EnggDiffGSASFittingViewQtWidget.h"
#include "EnggDiffGSASFittingModel.h"
#include "EnggDiffGSASFittingPresenter.h"

#include <QFileDialog>

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingViewQtWidget::EnggDiffGSASFittingViewQtWidget(
    boost::shared_ptr<IEnggDiffractionUserMsg> userMessageProvider)
    : m_userMessageProvider(userMessageProvider) {
  setupUI();

  m_zoomTool = Mantid::Kernel::make_unique<QwtPlotZoomer>(
      QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner,
      QwtPicker::AlwaysOff, m_ui.plotArea->canvas());
  m_zoomTool->setRubberBandPen(QPen(Qt::black));
  setZoomToolEnabled(false);

  auto model = Mantid::Kernel::make_unique<EnggDiffGSASFittingModel>();
  m_presenter = Mantid::Kernel::make_unique<EnggDiffGSASFittingPresenter>(
      std::move(model), this);
  m_presenter->notify(IEnggDiffGSASFittingPresenter::Start);
}

EnggDiffGSASFittingViewQtWidget::~EnggDiffGSASFittingViewQtWidget() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::ShutDown);

  for (auto &curves : m_focusedRunCurves) {
    curves->detach();
  }

  m_focusedRunCurves.clear();
}

void EnggDiffGSASFittingViewQtWidget::browseFocusedRun() {
  const auto filenames(
      QFileDialog::getOpenFileNames(this, tr("Find focused run files")));
  setFocusedRunFileNames(filenames);
}

void EnggDiffGSASFittingViewQtWidget::displayLatticeParams(
    const Mantid::API::ITableWorkspace_sptr latticeParams) const {
  UNUSED_ARG(latticeParams);
  throw std::runtime_error("displayLatticeParams not yet implemented");
}

void EnggDiffGSASFittingViewQtWidget::displayRwp(const double rwp) const {
  UNUSED_ARG(rwp);
  throw std::runtime_error("displayRwp not yet implemented");
}

std::vector<std::string>
EnggDiffGSASFittingViewQtWidget::getFocusedFileNames() const {
  const auto filenamesQStringList = m_ui.lineEdit_runFile->text().split(",");
  std::vector<std::string> filenames;

  for (const auto &filenameQString : filenamesQStringList) {
    filenames.push_back(filenameQString.toStdString());
  }
  return filenames;
}

std::string EnggDiffGSASFittingViewQtWidget::getGSASIIProjectPath() const {
  throw std::runtime_error("getGSASIIProjectPath not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getInstrumentFileName() const {
  throw std::runtime_error("getInstrumentFileName not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getPathToGSASII() const {
  throw std::runtime_error("getPathToGSASII not yet implemented");
}

double EnggDiffGSASFittingViewQtWidget::getPawleyDMin() const {
  throw std::runtime_error("getPawleyDMin not yet implemented");
}

double EnggDiffGSASFittingViewQtWidget::getPawleyNegativeWeight() const {
  throw std::runtime_error("getPawleyNegativeWeight not yet implemented");
}

std::vector<std::string>
EnggDiffGSASFittingViewQtWidget::getPhaseFileNames() const {
  throw std::runtime_error("getPhaseFileNames not yet implemented");
}

GSASRefinementMethod
EnggDiffGSASFittingViewQtWidget::getRefinementMethod() const {
  throw std::runtime_error("getRefinementMethod not yet implemented");
}

RunLabel EnggDiffGSASFittingViewQtWidget::getSelectedRunLabel() const {
  const auto currentItemLabel =
      m_ui.listWidget_runLabels->currentItem()->text();
  const auto pieces = currentItemLabel.split("_");
  if (pieces.size() != 2) {
    throw std::runtime_error(
        "Unexpected run label: \"" + currentItemLabel.toStdString() +
        "\". Please contact the development team with this message");
  }
  return RunLabel(pieces[0].toInt(), pieces[1].toUInt());
}

void EnggDiffGSASFittingViewQtWidget::loadFocusedRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
}

void EnggDiffGSASFittingViewQtWidget::plotCurve(
    const std::vector<boost::shared_ptr<QwtData>> &curves) {

  m_focusedRunCurves.reserve(curves.size());
  for (const auto &curve : curves) {
    auto plotCurve = Mantid::Kernel::make_unique<QwtPlotCurve>();

    plotCurve->setData(*curve);
    plotCurve->attach(m_ui.plotArea);
    m_focusedRunCurves.push_back(std::move(plotCurve));
  }

  m_ui.plotArea->replot();
  m_zoomTool->setZoomBase();
  setZoomToolEnabled(true);
}

void EnggDiffGSASFittingViewQtWidget::resetCanvas() {
  for (auto &curve : m_focusedRunCurves) {
    curve->detach();
  }
  m_focusedRunCurves.clear();
  resetPlotZoomLevel();
}

void EnggDiffGSASFittingViewQtWidget::resetPlotZoomLevel() {
  m_ui.plotArea->setAxisAutoScale(QwtPlot::xBottom);
  m_ui.plotArea->setAxisAutoScale(QwtPlot::yLeft);
  m_zoomTool->setZoomBase(true);
}

void EnggDiffGSASFittingViewQtWidget::selectRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
}

void EnggDiffGSASFittingViewQtWidget::setEnabled(const bool enabled) {
  m_ui.lineEdit_runFile->setEnabled(enabled);
  m_ui.pushButton_browseRunFile->setEnabled(enabled);
  m_ui.pushButton_loadRun->setEnabled(enabled);

  m_ui.lineEdit_instParamsFile->setEnabled(enabled);
  m_ui.pushButton_browseInstParams->setEnabled(enabled);

  m_ui.lineEdit_phaseFiles->setEnabled(enabled);
  m_ui.pushButton_browsePhaseFiles->setEnabled(enabled);

  m_ui.lineEdit_gsasProjPath->setEnabled(enabled);
  m_ui.pushButton_gsasProjPath->setEnabled(enabled);

  m_ui.lineEdit_gsasHome->setEnabled(enabled);
  m_ui.pushButton_browseGSASHome->setEnabled(enabled);

  m_ui.comboBox_refinementMethod->setEnabled(enabled);

  m_ui.lineEdit_pawleyDMin->setEnabled(enabled);
  m_ui.lineEdit_pawleyNegativeWeight->setEnabled(enabled);

  m_ui.checkBox_showRefinementResults->setEnabled(enabled);
}

void EnggDiffGSASFittingViewQtWidget::setFocusedRunFileNames(
    const QStringList &filenames) {
  m_ui.lineEdit_runFile->setText(filenames.join(tr(",")));
}

void EnggDiffGSASFittingViewQtWidget::setupUI() {
  m_ui.setupUi(this);
  connect(m_ui.pushButton_browseRunFile, SIGNAL(clicked()), this,
          SLOT(browseFocusedRun()));
  connect(m_ui.pushButton_loadRun, SIGNAL(clicked()), this,
          SLOT(loadFocusedRun()));
  connect(m_ui.listWidget_runLabels, SIGNAL(itemSelectionChanged()), this,
          SLOT(selectRun()));
}

void EnggDiffGSASFittingViewQtWidget::setZoomToolEnabled(const bool enabled) {
  m_zoomTool->setEnabled(enabled);
}

bool EnggDiffGSASFittingViewQtWidget::showRefinementResultsSelected() const {
  return m_ui.checkBox_showRefinementResults->isChecked();
}

void EnggDiffGSASFittingViewQtWidget::updateRunList(
    const std::vector<RunLabel> &runLabels) {
  m_ui.listWidget_runLabels->clear();
  for (const auto &runLabel : runLabels) {
    const auto labelStr = QString::number(runLabel.runNumber) + tr("_") +
                          QString::number(runLabel.bank);
    m_ui.listWidget_runLabels->addItem(labelStr);
  }
}

void EnggDiffGSASFittingViewQtWidget::userError(
    const std::string &errorTitle, const std::string &errorDescription) const {
  m_userMessageProvider->userError(errorTitle, errorDescription);
}

void EnggDiffGSASFittingViewQtWidget::userWarning(
    const std::string &warningTitle,
    const std::string &warningDescription) const {
  m_userMessageProvider->userWarning(warningTitle, warningDescription);
}

} // CustomInterfaces
} // MantidQt
