// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Sassena.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include <QFileInfo>
#include <QString>

namespace MantidQt::CustomInterfaces {
Sassena::Sassena(QWidget *parent) : SimulationTab(parent) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, PlotWidget::Spectra));

  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &Sassena::handleAlgorithmFinish);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &Sassena::saveClicked);
}

void Sassena::handleValidation(IUserInputValidator *validator) const {
  auto const inputFileName = m_uiForm.mwInputFile->getFirstFilename();
  if (inputFileName.isEmpty())
    validator->addErrorMessage("Incorrect input file provided.");
}

void Sassena::handleRun() {
  using namespace Mantid::API;
  using MantidQt::API::BatchAlgorithmRunner;

  clearOutputPlotOptionsWorkspaces();

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
void Sassena::handleAlgorithmFinish(bool error) {
  m_runPresenter->setRunEnabled(true);
  setSaveEnabled(!error);
  if (!error)
    setOutputPlotOptionsWorkspaces({m_outWsName.toStdString()});
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void Sassena::loadSettings(const QSettings &settings) { m_uiForm.mwInputFile->readSettings(settings.group()); }

/**
 * Handle saving of workspace
 */
void Sassena::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_outWsName.toStdString(), false))
    addSaveWorkspaceToQueue(m_outWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void Sassena::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace MantidQt::CustomInterfaces
