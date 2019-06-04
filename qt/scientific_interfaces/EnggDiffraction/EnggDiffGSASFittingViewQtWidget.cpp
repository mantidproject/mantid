// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffGSASFittingViewQtWidget.h"
#include "EnggDiffGSASFittingModel.h"
#include "EnggDiffGSASFittingPresenter.h"
#include "EnggDiffMultiRunFittingQtWidget.h"
#include "EnggDiffMultiRunFittingWidgetModel.h"
#include "EnggDiffMultiRunFittingWidgetPresenter.h"

#include "MantidAPI/TableRow.h"

#include <QFileDialog>
#include <QSettings>
#include <boost/make_shared.hpp>

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingViewQtWidget::EnggDiffGSASFittingViewQtWidget(
    boost::shared_ptr<IEnggDiffractionUserMsg> userMessageProvider,
    boost::shared_ptr<IEnggDiffractionPythonRunner> pythonRunner,
    boost::shared_ptr<IEnggDiffractionParam> mainSettings)
    : m_userMessageProvider(userMessageProvider) {

  auto multiRunWidgetModel =
      std::make_unique<EnggDiffMultiRunFittingWidgetModel>();
  m_multiRunWidgetView =
      std::make_unique<EnggDiffMultiRunFittingQtWidget>(
          pythonRunner);

  auto multiRunWidgetPresenter =
      boost::make_shared<EnggDiffMultiRunFittingWidgetPresenter>(
          std::move(multiRunWidgetModel), m_multiRunWidgetView.get());

  m_multiRunWidgetView->setPresenter(multiRunWidgetPresenter);
  m_multiRunWidgetView->setMessageProvider(m_userMessageProvider);

  setupUI();

  auto model = std::make_unique<EnggDiffGSASFittingModel>();
  auto *model_ptr = model.get();
  m_presenter = boost::make_shared<EnggDiffGSASFittingPresenter>(
      std::move(model), this, multiRunWidgetPresenter, mainSettings);
  model_ptr->setObserver(m_presenter);
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
  double length_a, length_b, length_c, angle_alpha, angle_beta, angle_gamma;
  Mantid::API::TableRow row = latticeParams->getFirstRow();
  row >> length_a >> length_b >> length_c >> angle_alpha >> angle_beta >>
      angle_gamma;
  m_ui.lineEdit_latticeParamA->setText(QString::number(length_a));
  m_ui.lineEdit_latticeParamB->setText(QString::number(length_b));
  m_ui.lineEdit_latticeParamC->setText(QString::number(length_c));
  m_ui.lineEdit_latticeParamAlpha->setText(QString::number(angle_alpha));
  m_ui.lineEdit_latticeParamBeta->setText(QString::number(angle_beta));
  m_ui.lineEdit_latticeParamGamma->setText(QString::number(angle_gamma));
}

void EnggDiffGSASFittingViewQtWidget::displayGamma(const double gamma) const {
  m_ui.lineEdit_gamma->setText(QString::number(gamma));
}

void EnggDiffGSASFittingViewQtWidget::displayRwp(const double rwp) const {
  m_ui.lineEdit_rwp->setText(QString::number(rwp));
}

void EnggDiffGSASFittingViewQtWidget::displaySigma(const double sigma) const {
  m_ui.lineEdit_sigma->setText(QString::number(sigma));
}

void EnggDiffGSASFittingViewQtWidget::doRefinement() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::DoRefinement);
}

std::vector<std::string>
EnggDiffGSASFittingViewQtWidget::getFocusedFileNames() const {
  const auto filenamesQStringList = m_ui.lineEdit_runFile->text().split(",");
  std::vector<std::string> filenames;
  filenames.reserve(filenamesQStringList.size());
  for (const auto &filenameQString : filenamesQStringList) {
    filenames.emplace_back(filenameQString.toStdString());
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

boost::optional<double> EnggDiffGSASFittingViewQtWidget::getPawleyDMin() const {
  const auto pawleyDMinString = m_ui.lineEdit_pawleyDMin->text();
  if (pawleyDMinString.isEmpty()) {
    return boost::none;
  }

  bool conversionSuccessful(false);
  const auto pawleyDMin = pawleyDMinString.toDouble(&conversionSuccessful);
  if (!conversionSuccessful) {
    userWarning("Invalid Pawley DMin", "Invalid entry for Pawley DMin \"" +
                                           pawleyDMinString.toStdString() +
                                           "\". Using default");
    return boost::none;
  }

  return pawleyDMin;
}

boost::optional<double>
EnggDiffGSASFittingViewQtWidget::getPawleyNegativeWeight() const {
  const auto pawleyNegWeightString = m_ui.lineEdit_pawleyNegativeWeight->text();
  if (pawleyNegWeightString.isEmpty()) {
    return boost::none;
  }

  bool conversionSuccessful(false);
  const auto pawleyNegWeight =
      pawleyNegWeightString.toDouble(&conversionSuccessful);
  if (!conversionSuccessful) {
    userWarning("Invalid Pawley negative weight",
                "Invalid entry for negative weight \"" +
                    pawleyNegWeightString.toStdString() + "\". Using default");
    return boost::none;
  }

  return pawleyNegWeight;
}

std::vector<std::string>
EnggDiffGSASFittingViewQtWidget::getPhaseFileNames() const {
  std::vector<std::string> fileNameStrings;
  const auto fileNameQStrings = m_ui.lineEdit_phaseFiles->text().split(",");
  fileNameStrings.reserve(fileNameQStrings.size());
  for (const auto &fileNameQString : fileNameQStrings) {
    fileNameStrings.emplace_back(fileNameQString.toStdString());
  }
  return fileNameStrings;
}

bool EnggDiffGSASFittingViewQtWidget::getRefineGamma() const {
  return m_ui.checkBox_refineGamma->isChecked();
}

bool EnggDiffGSASFittingViewQtWidget::getRefineSigma() const {
  return m_ui.checkBox_refineSigma->isChecked();
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

boost::optional<double> EnggDiffGSASFittingViewQtWidget::getXMax() const {
  const auto xMaxString = m_ui.lineEdit_xMax->text();
  if (xMaxString.isEmpty()) {
    return boost::none;
  }

  bool conversionSuccessful(false);
  const auto xMax = xMaxString.toDouble(&conversionSuccessful);
  if (!conversionSuccessful) {
    userWarning("Invalid XMax", "Invalid entry for XMax \"" +
                                    xMaxString.toStdString() +
                                    "\". Using default");
    return boost::none;
  }

  return xMax;
}

boost::optional<double> EnggDiffGSASFittingViewQtWidget::getXMin() const {
  const auto xMinString = m_ui.lineEdit_xMin->text();
  if (xMinString.isEmpty()) {
    return boost::none;
  }

  bool conversionSuccessful(false);
  const auto xMin = xMinString.toDouble(&conversionSuccessful);
  if (!conversionSuccessful) {
    userWarning("Invalid XMin", "Invalid entry for XMin \"" +
                                    xMinString.toStdString() +
                                    "\". Using default");
    return boost::none;
  }

  return xMin;
}

void EnggDiffGSASFittingViewQtWidget::loadFocusedRun() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
}

void EnggDiffGSASFittingViewQtWidget::refineAll() {
  m_presenter->notify(IEnggDiffGSASFittingPresenter::RefineAll);
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

  m_ui.lineEdit_xMin->setEnabled(enabled);
  m_ui.lineEdit_xMax->setEnabled(enabled);

  m_ui.checkBox_refineSigma->setEnabled(enabled);
  m_ui.checkBox_refineGamma->setEnabled(enabled);

  m_ui.pushButton_doRefinement->setEnabled(enabled);
  m_ui.pushButton_refineAll->setEnabled(enabled);

  m_multiRunWidgetView->setEnabled(enabled);
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
  connect(m_ui.pushButton_refineAll, SIGNAL(clicked()), this,
          SLOT(refineAll()));

  connect(m_multiRunWidgetView.get(), SIGNAL(runSelected()), this,
          SLOT(selectRun()));

  QSettings settings(tr(SETTINGS_NAME));
  if (settings.contains(tr(GSAS_HOME_SETTING_NAME))) {
    m_ui.lineEdit_gsasHome->setText(
        settings.value(tr(GSAS_HOME_SETTING_NAME)).toString());
  }
}

void EnggDiffGSASFittingViewQtWidget::showStatus(
    const std::string &status) const {
  m_userMessageProvider->showStatus(status);
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

} // namespace CustomInterfaces
} // namespace MantidQt
