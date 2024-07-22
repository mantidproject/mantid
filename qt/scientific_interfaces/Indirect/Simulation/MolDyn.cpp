// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MolDyn.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {
MolDyn::MolDyn(QWidget *parent) : SimulationTab(parent) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, PlotWidget::SpectraSliceSurface, "0"));

  connect(m_uiForm.ckCropEnergy, SIGNAL(toggled(bool)), m_uiForm.dspMaxEnergy, SLOT(setEnabled(bool)));
  connect(m_uiForm.ckResolution, SIGNAL(toggled(bool)), m_uiForm.dsResolution, SLOT(setEnabled(bool)));
  connect(m_uiForm.cbVersion, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(versionSelected(const QString &)));

  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsResolution->isOptional(true);
}

void MolDyn::handleValidation(IUserInputValidator *validator) const {
  if (validator->checkFileFinderWidgetIsValid("Data", m_uiForm.mwRun)) {
    QString filename = m_uiForm.mwRun->getFirstFilename();
    QString version = m_uiForm.cbVersion->currentText();
    QFileInfo finfo(filename);
    QString ext = finfo.suffix().toLower();

    if (version == "3") {
      if (ext != "dat" && ext != "cdl")
        validator->addErrorMessage("File is not of expected type.\n File type must be .dat or .cdl");

      QString functions = m_uiForm.leFunctionNames->text();
      if (ext == "cdl" && functions.isEmpty())
        validator->addErrorMessage("Must specify at least one function when loading CDL file.");
    }
  }

  // Validate resolution
  if (m_uiForm.ckResolution->isChecked())
    validator->checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);
}

void MolDyn::handleRun() {
  clearOutputPlotOptionsWorkspaces();

  // Get filename and base filename (for naming output workspace group)
  auto const filename = m_uiForm.mwRun->getFirstFilename();
  auto const functionNames = m_uiForm.leFunctionNames->text().toStdString();
  bool const symmetrise = m_uiForm.ckSymmetrise->isChecked();
  bool const cropEnergy = m_uiForm.ckCropEnergy->isChecked();
  bool const resolution = m_uiForm.ckResolution->isChecked();

  m_outputWsName = QFileInfo(filename).baseName().toStdString();

  // Setup algorithm
  auto molDynAlg = AlgorithmManager::Instance().create("MolDyn");
  molDynAlg->setProperty("Data", filename.toStdString());
  molDynAlg->setProperty("Functions", functionNames);
  molDynAlg->setProperty("SymmetriseEnergy", symmetrise);
  molDynAlg->setProperty("OutputWorkspace", m_outputWsName);

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

void MolDyn::algorithmComplete(bool error) {
  m_runPresenter->setRunEnabled(true);
  setSaveEnabled(!error);
  if (!error)
    setOutputPlotOptionsWorkspaces({m_outputWsName});
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void MolDyn::loadSettings(const QSettings &settings) { m_uiForm.mwRun->readSettings(settings.group()); }

/**
 * Handles the version of nMoldyn being selected.
 *
 * @param version The version as a string ("3" or "4")
 */
void MolDyn::versionSelected(const QString &version) {
  bool version4(version == "4");
  m_uiForm.mwRun->isForDirectory(version4);
}

/**
 * Handle saving workspaces
 */
void MolDyn::saveClicked() {

  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo fi(filename);
  QString baseName = fi.baseName();

  if (checkADSForPlotSaveWorkspace(baseName.toStdString(), false))
    addSaveWorkspaceToQueue(baseName);
  m_batchAlgoRunner->executeBatchAsync();
}

void MolDyn::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace MantidQt::CustomInterfaces
