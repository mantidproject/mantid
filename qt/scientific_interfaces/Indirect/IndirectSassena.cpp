// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSassena.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QFileInfo>
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {
IndirectSassena::IndirectSassena(QWidget *parent)
    : IndirectSimulationTab(parent) {
  m_uiForm.setupUi(parent);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(handleAlgorithmFinish(bool)));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
}

void IndirectSassena::setup() {}

/**
 * Validate the form to check the program can be run.
 *
 * @return Whether the form was valid
 */
bool IndirectSassena::validate() {
  UserInputValidator uiv;

  auto const inputFileName = m_uiForm.mwInputFile->getFirstFilename();
  if (inputFileName.isEmpty())
    uiv.addErrorMessage("Incorrect input file provided.");

  emit showMessageBox(uiv.generateErrorMessage());
  return uiv.isAllInputValid();
}

/**
 * Configures and executes the LoadSassena algorithm.
 */
void IndirectSassena::run() {
  using namespace Mantid::API;
  using MantidQt::API::BatchAlgorithmRunner;

  setRunIsRunning(true);

  QString const inputFileName = m_uiForm.mwInputFile->getFirstFilename();
  m_outWsName = QFileInfo(inputFileName).baseName();

  // If the workspace group already exists then remove it
  if (AnalysisDataService::Instance().doesExist(m_outWsName.toStdString()))
    AnalysisDataService::Instance().deepRemoveGroup(m_outWsName.toStdString());

  auto sassenaAlg = AlgorithmManager::Instance().create("LoadSassena");
  sassenaAlg->initialize();
  sassenaAlg->setProperty("Filename", inputFileName.toStdString());
  sassenaAlg->setProperty("SortByQVectors", m_uiForm.cbSortQ->isChecked());
  sassenaAlg->setProperty("TimeUnit", m_uiForm.sbTimeUnit->value());
  sassenaAlg->setProperty("OutputWorkspace", m_outWsName.toStdString());

  m_batchAlgoRunner->addAlgorithm(sassenaAlg);

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the algorithm batch.
 *
 * @param error If the batch was stopped due to error
 */
void IndirectSassena::handleAlgorithmFinish(bool error) {
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
void IndirectSassena::loadSettings(const QSettings &settings) {
  m_uiForm.mwInputFile->readSettings(settings.group());
}

void IndirectSassena::runClicked() { runTab(); }

/**
 * Handle mantid plotting of workspace
 */
void IndirectSassena::plotClicked() {
  setPlotIsPlotting(true);
  if (checkADSForPlotSaveWorkspace(m_outWsName.toStdString(), true))
    plotSpectrum(m_outWsName);
  setPlotIsPlotting(false);
}

/**
 * Handle saving of workspace
 */
void IndirectSassena::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_outWsName.toStdString(), false))
    addSaveWorkspaceToQueue(m_outWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectSassena::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void IndirectSassena::setPlotIsPlotting(bool running) {
  m_uiForm.pbPlot->setText(running ? "Plotting..." : "Plot Result");
  setButtonsEnabled(!running);
}

void IndirectSassena::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotEnabled(enabled);
  setSaveEnabled(enabled);
}

void IndirectSassena::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectSassena::setPlotEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
}

void IndirectSassena::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

} // namespace CustomInterfaces
} // namespace MantidQt
