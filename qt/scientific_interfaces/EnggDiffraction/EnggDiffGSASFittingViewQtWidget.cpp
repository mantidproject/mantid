#include "EnggDiffGSASFittingViewQtWidget.h"
#include "EnggDiffGSASFittingModel.h"
#include "EnggDiffGSASFittingPresenter.h"

#include <QFileDialog>

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingViewQtWidget::EnggDiffGSASFittingViewQtWidget() {
  setupUI();

  auto model = std::make_unique<EnggDiffGSASFittingModel>();
  m_presenter.reset(new EnggDiffGSASFittingPresenter(std::move(model), this));
  m_presenter->notify(IEnggDiffGSASFittingPresenter::Start);
}

EnggDiffGSASFittingViewQtWidget::~EnggDiffGSASFittingViewQtWidget() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::ShutDown);

  for (auto curves : m_focusedRunCurves) {
    curves->detach();
    delete curves;
  }
}

void EnggDiffGSASFittingViewQtWidget::browseFocusedRun() {
  QString path(QFileDialog::getOpenFileName(this, tr("Find focused run file")));
  setFocusedRunFileName(path);
}

void EnggDiffGSASFittingViewQtWidget::displayLatticeParams(
    const Mantid::API::ITableWorkspace_sptr latticeParams) const {
  (void)latticeParams;
  throw std::runtime_error("displayLatticeParams not yet implemented");
}

void EnggDiffGSASFittingViewQtWidget::displayRwp(const double rwp) const {
  (void)rwp;
  throw std::runtime_error("displayRwp not yet implemented");
}

std::string EnggDiffGSASFittingViewQtWidget::getFocusedFileName() const {
  return m_ui.lineEdit_runFile->text().toStdString();
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

std::pair<int, size_t>
EnggDiffGSASFittingViewQtWidget::getSelectedRunLabel() const {
  const auto currentItemLabel =
      m_ui.listWidget_runLabels->currentItem()->text();
  const auto pieces = currentItemLabel.split("_");
  return std::make_pair(pieces[0].toInt(), pieces[1].toUInt());
}

void EnggDiffGSASFittingViewQtWidget::loadFocusedRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
}

void EnggDiffGSASFittingViewQtWidget::plotCurve(
    const std::vector<boost::shared_ptr<QwtData>> &curves) {

  for (size_t i = 0; i < curves.size(); ++i) {
    auto *curve = curves[i].get();
    QwtPlotCurve *plotCurve = new QwtPlotCurve();

    m_focusedRunCurves.push_back(plotCurve);
    m_focusedRunCurves[i]->setData(*curve);
    m_focusedRunCurves[i]->attach(m_ui.plotArea);
  }

  m_ui.plotArea->replot();
}

void EnggDiffGSASFittingViewQtWidget::resetCanvas() {
  for (auto curve : m_focusedRunCurves) {
    if (curve) {
      curve->detach();
      delete curve;
    }
  }

  if (m_focusedRunCurves.size() > 0) {
    m_focusedRunCurves.clear();
  }
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

void EnggDiffGSASFittingViewQtWidget::setFocusedRunFileName(
    const QString &filename) {
  m_ui.lineEdit_runFile->setText(filename);
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

bool EnggDiffGSASFittingViewQtWidget::showRefinementResultsSelected() const {
  return m_ui.checkBox_showRefinementResults->isChecked();
}

void EnggDiffGSASFittingViewQtWidget::updateRunList(
    const std::vector<std::pair<int, size_t>> &runLabels) {
  m_ui.listWidget_runLabels->clear();
  for (const auto &runLabel : runLabels) {
    const auto labelStr = QString::number(runLabel.first) + tr("_") +
                          QString::number(runLabel.second);
    m_ui.listWidget_runLabels->addItem(labelStr);
  }
}

void EnggDiffGSASFittingViewQtWidget::userWarning(
    const std::string &warningDescription) const {
  (void)warningDescription;
  throw std::runtime_error("userWarning not yet implemented");
}

} // CustomInterfaces
} // MantidQt
