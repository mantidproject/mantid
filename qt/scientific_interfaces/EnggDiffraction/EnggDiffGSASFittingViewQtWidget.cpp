#include "EnggDiffGSASFittingViewQtWidget.h"
#include "EnggDiffGSASFittingModel.h"
#include "EnggDiffGSASFittingPresenter.h"
#include "EnggDiffMultiRunFittingQtWidget.h"
#include "EnggDiffMultiRunFittingWidgetModel.h"
#include "EnggDiffMultiRunFittingWidgetPresenter.h"

#include <boost/make_shared.hpp>

#include <QFileDialog>

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingViewQtWidget::EnggDiffGSASFittingViewQtWidget(
    boost::shared_ptr<IEnggDiffractionUserMsg> userMessageProvider,
    boost::shared_ptr<IEnggDiffractionPythonRunner> pythonRunner)
    : m_userMessageProvider(userMessageProvider) {

  auto multiRunWidgetModel =
      Mantid::Kernel::make_unique<EnggDiffMultiRunFittingWidgetModel>();
  m_multiRunWidgetView =
      Mantid::Kernel::make_unique<EnggDiffMultiRunFittingQtWidget>(
          pythonRunner);

  auto multiRunWidgetPresenter =
      boost::make_shared<EnggDiffMultiRunFittingWidgetPresenter>(
          std::move(multiRunWidgetModel), m_multiRunWidgetView.get());

  m_multiRunWidgetView->setPresenter(multiRunWidgetPresenter);
  m_multiRunWidgetView->setMessageProvider(m_userMessageProvider);

  setupUI();

  auto model = Mantid::Kernel::make_unique<EnggDiffGSASFittingModel>();
  m_presenter = Mantid::Kernel::make_unique<EnggDiffGSASFittingPresenter>(
      std::move(model), this, multiRunWidgetPresenter);
  m_presenter->notify(IEnggDiffGSASFittingPresenter::Start);
}

EnggDiffGSASFittingViewQtWidget::~EnggDiffGSASFittingViewQtWidget() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::ShutDown);
}

void EnggDiffGSASFittingViewQtWidget::addWidget(
    IEnggDiffMultiRunFittingWidgetView *widget) {
  QWidget *qWidget = dynamic_cast<QWidget *>(widget);
  m_ui.gridLayout_multiRunWidget->addWidget(qWidget, 0, 0);
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

void EnggDiffGSASFittingViewQtWidget::loadFocusedRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
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

  connect(m_multiRunWidgetView.get(), SIGNAL(runSelected()), this,
          SLOT(selectRun()));
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
