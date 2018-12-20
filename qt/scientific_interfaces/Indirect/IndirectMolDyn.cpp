// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectMolDyn.h"

#include "../General/UserInputValidator.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

namespace {

WorkspaceGroup_sptr getADSWorkspaceGroup(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
IndirectMolDyn::IndirectMolDyn(QWidget *parent)
    : IndirectSimulationTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.ckCropEnergy, SIGNAL(toggled(bool)), m_uiForm.dspMaxEnergy,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.ckResolution, SIGNAL(toggled(bool)), m_uiForm.dsResolution,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.cbVersion, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(versionSelected(const QString &)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
}

void IndirectMolDyn::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool IndirectMolDyn::validate() {
  UserInputValidator uiv;

  if (uiv.checkMWRunFilesIsValid("Data", m_uiForm.mwRun)) {
    QString filename = m_uiForm.mwRun->getFirstFilename();
    QString version = m_uiForm.cbVersion->currentText();
    QFileInfo finfo(filename);
    QString ext = finfo.suffix().toLower();

    if (version == "3") {
      if (ext != "dat" && ext != "cdl")
        uiv.addErrorMessage(
            "File is not of expected type.\n File type must be .dat or .cdl");

      QString functions = m_uiForm.leFunctionNames->text();
      if (ext == "cdl" && functions.isEmpty())
        uiv.addErrorMessage(
            "Must specify at least one function when loading CDL file.");
    }
  }

  // Validate resolution
  if (m_uiForm.ckResolution->isChecked())
    uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  emit showMessageBox(uiv.generateErrorMessage());
  return uiv.isAllInputValid();
}

/**
 * Collect the settings on the GUI and run the MolDyn algorithm.
 */
void IndirectMolDyn::run() {
  setRunIsRunning(true);

  // Get filename and base filename (for naming output workspace group)
  auto const filename = m_uiForm.mwRun->getFirstFilename();
  auto const baseName = QFileInfo(filename).baseName();
  auto const functionNames = m_uiForm.leFunctionNames->text().toStdString();
  bool const symmetrise = m_uiForm.ckSymmetrise->isChecked();
  bool const cropEnergy = m_uiForm.ckCropEnergy->isChecked();
  bool const resolution = m_uiForm.ckResolution->isChecked();

  // Setup algorithm
  auto molDynAlg = AlgorithmManager::Instance().create("MolDyn");
  molDynAlg->setProperty("Data", filename.toStdString());
  molDynAlg->setProperty("Functions", functionNames);
  molDynAlg->setProperty("SymmetriseEnergy", symmetrise);
  molDynAlg->setProperty("OutputWorkspace", baseName.toStdString());

  // Set energy crop option
  if (cropEnergy) {
    auto const maxEnergy = QString::number(m_uiForm.dspMaxEnergy->value());
    molDynAlg->setProperty("MaxEnergy", maxEnergy.toStdString());
  }

  // Set instrument resolution option
  if (resolution) {
    auto const resolutionName = m_uiForm.dsResolution->getCurrentDataName();
    molDynAlg->setProperty("Resolution", resolutionName.toStdString());
  }

  runAlgorithm(molDynAlg);
}

void IndirectMolDyn::algorithmComplete(bool error) {
  setRunIsRunning(false);
  if (error) {
    setPlotEnabled(false);
    setSaveEnabled(false);
  }
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void IndirectMolDyn::loadSettings(const QSettings &settings) {
  m_uiForm.mwRun->readSettings(settings.group());
}

/**
 * Handles the version of nMoldyn being selected.
 *
 * @param version The version as a string ("3" or "4")
 */
void IndirectMolDyn::versionSelected(const QString &version) {
  bool version4(version == "4");
  m_uiForm.mwRun->isForDirectory(version4);
}

void IndirectMolDyn::runClicked() { runTab(); }

/**
 * Handle plotting of mantid workspace
 */
void IndirectMolDyn::plotClicked() {
  setPlotIsPlotting(true);

  QString const filename = m_uiForm.mwRun->getFirstFilename();
  QString const baseName = QFileInfo(filename).baseName();

  if (checkADSForPlotSaveWorkspace(baseName.toStdString(), true)) {

    auto const diffResultsGroup = getADSWorkspaceGroup(baseName.toStdString());
    auto const names = diffResultsGroup->getNames();
    auto const plotType = m_uiForm.cbPlot->currentText();

    for (auto const &name : names) {
      if (plotType == "Spectra" || plotType == "Both")
        plotSpectrum(QString::fromStdString(name));

      if (plotType == "Contour" || plotType == "Both")
        plot2D(QString::fromStdString(name));
    }
  }

  setPlotIsPlotting(false);
}

/**
 * Handle saving workspaces
 */
void IndirectMolDyn::saveClicked() {

  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo fi(filename);
  QString baseName = fi.baseName();

  if (checkADSForPlotSaveWorkspace(baseName.toStdString(), false))
    addSaveWorkspaceToQueue(baseName);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectMolDyn::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void IndirectMolDyn::setPlotIsPlotting(bool running) {
  m_uiForm.pbPlot->setText(running ? "Plotting..." : "Plot");
  setButtonsEnabled(!running);
}

void IndirectMolDyn::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotEnabled(enabled);
  setSaveEnabled(enabled);
}

void IndirectMolDyn::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectMolDyn::setPlotEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void IndirectMolDyn::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

} // namespace CustomInterfaces
} // namespace MantidQt
