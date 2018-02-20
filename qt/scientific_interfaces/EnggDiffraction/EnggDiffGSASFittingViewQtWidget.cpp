#include "EnggDiffGSASFittingViewQtWidget.h"
#include "EnggDiffGSASFittingModel.h"
#include "EnggDiffGSASFittingPresenter.h"
#include "EnggDiffMultiRunFittingQtWidget.h"
#include "EnggDiffMultiRunFittingWidgetModel.h"
#include "EnggDiffMultiRunFittingWidgetPresenter.h"

#include <boost/make_shared.hpp>

#include <QFileDialog>
#include <QSettings>

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
  QSettings settings(tr(SETTINGS_NAME));
  const auto gsasHome = m_ui.lineEdit_gsasHome->text();
  settings.setValue(tr(GSAS_HOME_SETTING_NAME), gsasHome);

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

void EnggDiffGSASFittingViewQtWidget::browseGSASHome() {
  auto directoryName(QFileDialog::getExistingDirectory(
      this, tr("GSAS-II installation directory")));
  m_ui.lineEdit_gsasHome->setText(directoryName);
}

void EnggDiffGSASFittingViewQtWidget::browseGSASProj() {
  auto filename(QFileDialog::getSaveFileName(
      this, tr("Output GSAS-II project file"), "", "GSAS-II Project (*.gpx)"));
  if (!filename.endsWith(".gpx")) {
    filename.append(".gpx");
  }
  m_ui.lineEdit_gsasProjPath->setText(filename);
}

void EnggDiffGSASFittingViewQtWidget::browseInstParams() {
  const auto filename(
      QFileDialog::getOpenFileName(this, tr("Instrument parameter file"), "",
                                   "Instrument parameter file (*.par *.prm)"));
  m_ui.lineEdit_instParamsFile->setText(filename);
}

void EnggDiffGSASFittingViewQtWidget::browsePhaseFiles() {
  const auto filenames(QFileDialog::getOpenFileNames(
      this, tr("Phase files"), "", "Phase files (*.cif)"));
  m_ui.lineEdit_phaseFiles->setText(filenames.join(tr(",")));
}

void EnggDiffGSASFittingViewQtWidget::disableLoadIfInputEmpty() {
  setLoadEnabled(!runFileLineEditEmpty());
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

void EnggDiffGSASFittingViewQtWidget::doRefinement() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::DoRefinement);
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
  return m_ui.lineEdit_gsasProjPath->text().toStdString();
}

std::string EnggDiffGSASFittingViewQtWidget::getInstrumentFileName() const {
  return m_ui.lineEdit_instParamsFile->text().toStdString();
}

std::string EnggDiffGSASFittingViewQtWidget::getPathToGSASII() const {
  return m_ui.lineEdit_gsasHome->text().toStdString();
}

double EnggDiffGSASFittingViewQtWidget::getPawleyDMin() const {
  const auto pawleyDMinString = m_ui.lineEdit_pawleyDMin->text();
  bool conversionSuccessful(false);
  const auto pawleyDMin = pawleyDMinString.toDouble(&conversionSuccessful);
  if (conversionSuccessful) {
    return pawleyDMin;
  } else {
    userWarning("Invalid Pawley DMin", "Invalid entry for Pawley DMin \"" +
                                           pawleyDMinString.toStdString() +
                                           "\". Using default (1");
    return 1;
  }
}

double EnggDiffGSASFittingViewQtWidget::getPawleyNegativeWeight() const {
  const auto pawleyNegWeightString = m_ui.lineEdit_pawleyNegativeWeight->text();
  bool conversionSuccessful(false);
  const auto pawleyNegWeight =
      pawleyNegWeightString.toDouble(&conversionSuccessful);
  if (conversionSuccessful) {
    return pawleyNegWeight;
  } else {
    userWarning("Invalid Pawley negative weight",
                "Invalid entry for negative weight \"" +
                    pawleyNegWeightString.toStdString() +
                    "\". Using default (0)");
    return 0;
  }
}

std::vector<std::string>
EnggDiffGSASFittingViewQtWidget::getPhaseFileNames() const {
  std::vector<std::string> fileNameStrings;
  const auto fileNameQStrings = m_ui.lineEdit_phaseFiles->text().split(",");
  fileNameStrings.reserve(fileNameQStrings.size());
  for (const auto &fileNameQString : fileNameQStrings) {
    fileNameStrings.push_back(fileNameQString.toStdString());
  }
  return fileNameStrings;
}

GSASRefinementMethod
EnggDiffGSASFittingViewQtWidget::getRefinementMethod() const {
  const auto refinementMethod =
      m_ui.comboBox_refinementMethod->currentText().toStdString();
  if (refinementMethod == "Pawley") {
    return GSASRefinementMethod::PAWLEY;
  } else if (refinementMethod == "Rietveld") {
    return GSASRefinementMethod::RIETVELD;
  } else {
    userError("Unexpected refinement method",
              "Unexpected refinement method \"" + refinementMethod +
                  "\" selected. Please contact development team with this "
                  "message. If you choose to continue, Pawley will be used");
    return GSASRefinementMethod::PAWLEY;
  }
}

void EnggDiffGSASFittingViewQtWidget::loadFocusedRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
}

bool EnggDiffGSASFittingViewQtWidget::runFileLineEditEmpty() const {
  return m_ui.lineEdit_runFile->text().isEmpty();
}

void EnggDiffGSASFittingViewQtWidget::selectRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
}

void EnggDiffGSASFittingViewQtWidget::setEnabled(const bool enabled) {
  m_ui.lineEdit_runFile->setEnabled(enabled);
  m_ui.pushButton_browseRunFile->setEnabled(enabled);
  setLoadEnabled(enabled && !runFileLineEditEmpty());

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

void EnggDiffGSASFittingViewQtWidget::setLoadEnabled(const bool enabled) {
  if (enabled) {
    m_ui.pushButton_loadRun->setEnabled(true);
    m_ui.pushButton_loadRun->setToolTip(tr("Load focused run file"));
  } else {
    m_ui.pushButton_loadRun->setEnabled(false);
    m_ui.pushButton_loadRun->setToolTip(
        tr("Please specify a file to load via the browse menu or by typing the "
           "full path to the file in the text field"));
  }
}

void EnggDiffGSASFittingViewQtWidget::setupUI() {
  m_ui.setupUi(this);
  connect(m_ui.pushButton_browseRunFile, SIGNAL(clicked()), this,
          SLOT(browseFocusedRun()));
  connect(m_ui.pushButton_loadRun, SIGNAL(clicked()), this,
          SLOT(loadFocusedRun()));
  connect(m_ui.lineEdit_runFile, SIGNAL(textChanged(const QString &)), this,
          SLOT(disableLoadIfInputEmpty()));

  connect(m_ui.pushButton_browseInstParams, SIGNAL(clicked()), this,
          SLOT(browseInstParams()));
  connect(m_ui.pushButton_browsePhaseFiles, SIGNAL(clicked()), this,
          SLOT(browsePhaseFiles()));
  connect(m_ui.pushButton_gsasProjPath, SIGNAL(clicked()), this,
          SLOT(browseGSASProj()));
  connect(m_ui.pushButton_browseGSASHome, SIGNAL(clicked()), this,
          SLOT(browseGSASHome()));

  connect(m_ui.pushButton_doRefinement, SIGNAL(clicked()), this,
          SLOT(doRefinement()));

  connect(m_multiRunWidgetView.get(), SIGNAL(runSelected()), this,
          SLOT(selectRun()));

  QSettings settings(tr(SETTINGS_NAME));
  if (settings.contains(tr(GSAS_HOME_SETTING_NAME))) {
    m_ui.lineEdit_gsasHome->setText(
        settings.value(tr(GSAS_HOME_SETTING_NAME)).toString());
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

const char EnggDiffGSASFittingViewQtWidget::GSAS_HOME_SETTING_NAME[] =
    "GSAS_HOME";
const char EnggDiffGSASFittingViewQtWidget::SETTINGS_NAME[] =
    "EnggGUIGSASTabSettings";

} // CustomInterfaces
} // MantidQt
