// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DensityOfStates.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("DensityOfStates");
} // namespace

namespace MantidQt::CustomInterfaces {

enum class DensityOfStates::InputFormat : int { Unsupported = 0, Phonon, Castep, ForceConstants };

DensityOfStates::DensityOfStates(QWidget *parent) : SimulationTab(parent) {
  m_uiForm.setupUi(parent);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));
  setOutputPlotOptionsPresenter(m_uiForm.ipoPlotOptions, PlotWidget::Spectra);

  connect(m_uiForm.mwInputFile, &FileFinderWidget::filesFound, this, &DensityOfStates::handleFileChange);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &DensityOfStates::saveClicked);

  m_uiForm.lwIons->setSelectionMode(QAbstractItemView::MultiSelection);
}

void DensityOfStates::handleValidation(IUserInputValidator *validator) const {
  auto const filename = m_uiForm.mwInputFile->getFirstFilename().toStdString();
  if (filename.empty()) {
    validator->addErrorMessage("A data file has not been loaded.");
    return;
  }
  auto const format = filenameToFormat(filename);
  if (format == InputFormat::Unsupported) {
    validator->addErrorMessage("The provided file format is unsupported. The supported extensions are 'phonon', "
                               "'castep', 'castep_bin' and 'yaml'.");
    return;
  }

  auto const specType = m_uiForm.cbSpectrumType->currentText();
  auto const items = m_uiForm.lwIons->selectedItems();

  if (specType == "DensityOfStates" && isPdosFile(format) && items.size() < 1)
    validator->addErrorMessage("Must select at least one ion for DensityOfStates.");
}

void DensityOfStates::handleRun() {
  clearOutputPlotOptionsWorkspaces();

  // Get the SimulatedDensityOfStates algorithm
  auto dosAlgo = AlgorithmManager::Instance().create("SimulatedDensityOfStates");

  const auto filename = m_uiForm.mwInputFile->getFirstFilename().toStdString();
  const auto specType = m_uiForm.cbSpectrumType->currentText();
  const auto filePropName = formatToFilePropName(filenameToFormat(filename));

  m_outputWsName = QFileInfo(QString::fromStdString(filename)).baseName() + "_" + specType;

  // Set common properties
  dosAlgo->setProperty(filePropName, filename);
  dosAlgo->setProperty("OutputWorkspace", m_outputWsName.toStdString());

  const auto peakShape = m_uiForm.cbPeakShape->currentText().toStdString();
  dosAlgo->setProperty("Function", peakShape);

  const auto peakWidth = m_uiForm.spPeakWidth->text().toStdString();
  dosAlgo->setProperty("PeakWidth", peakWidth);

  const auto binWidth = m_uiForm.spBinWidth->value();
  dosAlgo->setProperty("BinWidth", binWidth);

  const auto zeroThreshold = m_uiForm.spZeroThreshold->value();
  dosAlgo->setProperty("ZeroThreshold", zeroThreshold);

  const auto scale = m_uiForm.ckScale->isChecked();
  if (scale) {
    const auto scaleFactor = m_uiForm.spScale->value();
    dosAlgo->setProperty("Scale", scaleFactor);
  }

  // Set spectrum type specific properties
  if (specType == "DensityOfStates") {
    dosAlgo->setProperty("SpectrumType", "DOS");

    const auto crossSectionScale = m_uiForm.ckCrossSectionScale->isChecked();
    const auto crossSectionScaleType = m_uiForm.cbCrossSectionScale->currentText().toStdString();
    if (crossSectionScale)
      dosAlgo->setProperty("ScaleByCrossSection", crossSectionScaleType);

    const auto outputFormat = m_uiForm.cbOutputFormat->currentIndex();
    if (outputFormat == 1) {
      dosAlgo->setProperty("SumContributions", true);
    }
    if (outputFormat == 2) {
      dosAlgo->setProperty("CalculateIonIndices", true);
    }

    std::vector<std::string> selectedIons;
    auto items = m_uiForm.lwIons->selectedItems();
    selectedIons.reserve(items.size());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(selectedIons),
                   [](const auto &item) { return item->text().toStdString(); });

    dosAlgo->setProperty("Ions", selectedIons);
  } else if (specType == "IR") {
    dosAlgo->setProperty("SpectrumType", "IR_Active");
  } else if (specType == "Raman") {
    dosAlgo->setProperty("SpectrumType", "Raman_Active");

    const auto temperature = m_uiForm.spTemperature->value();
    dosAlgo->setProperty("Temperature", temperature);
  }

  m_batchAlgoRunner->addAlgorithm(dosAlgo);

  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &DensityOfStates::dosAlgoComplete);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the SimulatedDensityOfStates algorithm.
 *
 * @param error If the algorithm failed
 */
void DensityOfStates::dosAlgoComplete(bool error) {
  disconnect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &DensityOfStates::dosAlgoComplete);

  m_runPresenter->setRunEnabled(true);
  setSaveEnabled(!error);
  if (!error)
    setOutputPlotOptionsWorkspaces({m_outputWsName.toStdString()});
}

/**
 * Handles a new file being selected by the browser.
 */
void DensityOfStates::handleFileChange() {
  auto const filename = m_uiForm.mwInputFile->getFirstFilename().toStdString();
  InputFormat fileFormat = filenameToFormat(filename);
  bool pdosAvailable = isPdosFile(fileFormat);

  if (pdosAvailable) {
    // Load the ion table to populate the list of ions
    IAlgorithm_sptr ionTableAlgo = AlgorithmManager::Instance().create("SimulatedDensityOfStates");
    ionTableAlgo->initialize();
    ionTableAlgo->setProperty(formatToFilePropName(fileFormat), filename);
    ionTableAlgo->setProperty("SpectrumType", "IonTable");
    ionTableAlgo->setProperty("OutputWorkspace", "__dos_ions");

    m_batchAlgoRunner->addAlgorithm(ionTableAlgo);

    connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &DensityOfStates::ionLoadComplete);
    m_batchAlgoRunner->executeBatchAsync();
  } else {
    m_uiForm.lwIons->clear();
    m_uiForm.ckCrossSectionScale->setChecked(false);
  }

  // Enable partial DOS related optons when they can be used
  m_uiForm.lwIons->setEnabled(pdosAvailable);
  m_uiForm.pbSelectAllIons->setEnabled(pdosAvailable);
  m_uiForm.pbDeselectAllIons->setEnabled(pdosAvailable);
  m_uiForm.ckCrossSectionScale->setEnabled(pdosAvailable);
}

/**
 * Handles the algorithm loading the list of ions in a file
 * being completed.
 *
 * @param error If the algorithm failed
 */
void DensityOfStates::ionLoadComplete(bool error) {
  disconnect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &DensityOfStates::ionLoadComplete);

  if (error) {
    g_log.error("Could not get a list of ions from input file");
  } else {

    // Get the list of ions from algorithm
    auto ionTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__dos_ions");
    Column_sptr ionColumn = ionTable->getColumn("Species");
    size_t numIons = ionColumn->size();

    // Remove old ions
    m_uiForm.lwIons->clear();

    // Add ions to list
    QStringList ionSpecies;
    for (size_t ion = 0; ion < numIons; ion++) {
      const QString species = QString::fromStdString(ionColumn->cell<std::string>(ion));
      if (!ionSpecies.contains(species))
        ionSpecies << species;
    }
    m_uiForm.lwIons->addItems(ionSpecies);

    // Select all ions by default
    m_uiForm.lwIons->selectAll();
  }
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void DensityOfStates::loadSettings(const QSettings &settings) { m_uiForm.mwInputFile->readSettings(settings.group()); }

/**
 * Handle saving of workspace
 */
void DensityOfStates::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_outputWsName.toStdString(), false))
    addSaveWorkspaceToQueue(m_outputWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void DensityOfStates::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

DensityOfStates::InputFormat DensityOfStates::filenameToFormat(std::string const &filename) const {
  QFileInfo inputFileInfo(QString::fromStdString(filename));
  const auto suffix = inputFileInfo.suffix().toStdString();

  InputFormat format;

  if (suffix == "phonon") {
    format = InputFormat::Phonon;
  } else if (suffix == "castep") {
    format = InputFormat::Castep;
  } else if (suffix == "castep_bin") {
    format = InputFormat::ForceConstants;
  } else if (suffix == "yaml") {
    format = InputFormat::ForceConstants;
  } else {
    format = InputFormat::Unsupported;
  }

  return format;
}

std::string DensityOfStates::formatToFilePropName(InputFormat const &format) const {
  std::string filePropName;

  switch (format) {
  case InputFormat::Phonon:
    filePropName = "PHONONFile";
    break;
  case InputFormat::Castep:
    filePropName = "CASTEPFile";
    break;
  case InputFormat::ForceConstants:
    filePropName = "ForceConstantsFile";
    break;
  default:
    g_log.error("Could not determine appropriate input field for this file type. ");
  }

  return filePropName;
}

bool DensityOfStates::isPdosFile(InputFormat const &dosFileFormat) const {
  return (dosFileFormat == InputFormat::Phonon) || (dosFileFormat == InputFormat::ForceConstants);
}

} // namespace MantidQt::CustomInterfaces
