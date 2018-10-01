#include "ISISEnergyTransfer.h"

#include "../General/UserInputValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QFileInfo>

#include <boost/algorithm/string.hpp>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace {

std::string createRangeString(std::size_t from, std::size_t to) {
  return std::to_string(from) + "-" + std::to_string(to);
}

std::string createGroupString(std::size_t start, std::size_t size) {
  return createRangeString(start, start + size - 1);
}

std::string createGroupingString(std::size_t groupSize,
                                 std::size_t numberOfGroups) {
  auto groupingString = createRangeString(0, groupSize - 1);
  for (auto i = groupSize; i < groupSize * numberOfGroups; i += groupSize)
    groupingString += "," + createGroupString(i, groupSize);
  return groupingString;
}

std::string createDetectorGroupingString(std::size_t groupSize,
                                         std::size_t numberOfGroups,
                                         std::size_t numberOfDetectors) {
  const auto groupingString = createGroupingString(groupSize, numberOfGroups);
  const auto remainder = numberOfDetectors % numberOfGroups;
  if (remainder == 0)
    return groupingString;
  return groupingString + "," +
         createRangeString(numberOfDetectors - remainder,
                           numberOfDetectors - 1);
}

std::string createDetectorGroupingString(std::size_t numberOfDetectors,
                                         std::size_t numberOfGroups) {
  const auto groupSize = numberOfDetectors / numberOfGroups;
  if (groupSize == 0)
    return createRangeString(0, numberOfDetectors - 1);
  return createDetectorGroupingString(groupSize, numberOfGroups,
                                      numberOfDetectors);
}

std::vector<std::size_t>
getCustomGroupingNumbers(std::string const &customString) {
  std::vector<std::string> customGroupingStrings;
  std::vector<std::size_t> customGroupingNumbers;
  // Get the numbers from customString and store them in customGroupingStrings
  boost::split(customGroupingStrings, customString, boost::is_any_of(" ,-+:"));
  for (const auto &string : customGroupingStrings)
    if (!string.empty())
      customGroupingNumbers.emplace_back(std::stoull(string));
  return customGroupingNumbers;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
ISISEnergyTransfer::ISISEnergyTransfer(IndirectDataReduction *idrUI,
                                       QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent) {
  m_uiForm.setupUi(parent);

  // SIGNAL/SLOT CONNECTIONS
  // Update instrument information when a new instrument config is selected
  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(setInstrumentDefault()));
  // Shows required mapping option UI widgets when a new mapping option is
  // selected from drop down
  connect(m_uiForm.cbGroupingOptions,
          SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(mappingOptionSelected(const QString &)));
  // Plots raw input data when user clicks Plot Time
  connect(m_uiForm.pbPlotTime, SIGNAL(clicked()), this, SLOT(plotRaw()));
  // Shows message on run button when user is inputting a run number
  connect(m_uiForm.dsRunFiles, SIGNAL(fileTextChanged(const QString &)), this,
          SLOT(pbRunEditing()));
  // Shows message on run button when Mantid is finding the file for a given run
  // number
  connect(m_uiForm.dsRunFiles, SIGNAL(findingFiles()), this,
          SLOT(pbRunFinding()));
  // Reverts run button back to normal when file finding has finished
  connect(m_uiForm.dsRunFiles, SIGNAL(fileFindingFinished()), this,
          SLOT(pbRunFinished()));
  // Handle plotting and saving
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  // Update UI widgets to show default values
  mappingOptionSelected(m_uiForm.cbGroupingOptions->currentText());

  // Add validation to custom detector grouping
  QRegExp re("([0-9]+[-:+]?[0-9]*,[ ]?)*[0-9]+[-:+]?[0-9]*");
  m_uiForm.leCustomGroups->setValidator(new QRegExpValidator(re, this));

  // Validate to remove invalid markers
  validateTab();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ISISEnergyTransfer::~ISISEnergyTransfer() {}

void ISISEnergyTransfer::setup() {}

bool ISISEnergyTransfer::validate() {
  UserInputValidator uiv;

  // Run files input
  if (!m_uiForm.dsRunFiles->isValid()) {
    uiv.addErrorMessage("Run file range is invalid.");
  }

  // Calibration file input
  if (m_uiForm.ckUseCalib->isChecked() &&
      !m_uiForm.dsCalibrationFile->isValid()) {
    uiv.addErrorMessage("Calibration file/workspace is invalid.");
  }

  QString groupingError = validateDetectorGrouping();
  if (!groupingError.isEmpty())
    uiv.addErrorMessage(groupingError);

  // Rebinning
  if (!m_uiForm.ckDoNotRebin->isChecked()) {
    if (m_uiForm.cbRebinType->currentText() == "Single") {
      double rebinWidth = m_uiForm.spRebinWidth->value();
      if (rebinWidth < 0) {
        // Ensure negative bin width is intentionally logarithmic
        const char *text =
            "The Binning width is currently negative, this suggests "
            "you wish to use logarithmic binning.\n"
            " Do you want to use Logarithmic Binning?";
        int result = QMessageBox::question(
            nullptr, tr("Logarithmic Binning"), tr(text), QMessageBox::Yes,
            QMessageBox::No, QMessageBox::NoButton);
        if (result == QMessageBox::Yes) {
          // Treat rebin width as a positive for validation
          rebinWidth = std::abs(rebinWidth);
        }
      }
      bool rebinValid = !uiv.checkBins(m_uiForm.spRebinLow->value(), rebinWidth,
                                       m_uiForm.spRebinHigh->value());
      m_uiForm.valRebinLow->setVisible(rebinValid);
      m_uiForm.valRebinWidth->setVisible(rebinValid);
      m_uiForm.valRebinHigh->setVisible(rebinValid);
    } else {
      uiv.checkFieldIsNotEmpty("Rebin string", m_uiForm.leRebinString,
                               m_uiForm.valRebinString);
    }
  } else {
    m_uiForm.valRebinLow->setVisible(false);
    m_uiForm.valRebinWidth->setVisible(false);
    m_uiForm.valRebinHigh->setVisible(false);
    m_uiForm.valRebinString->setVisible(false);
  }

  // DetailedBalance
  if (m_uiForm.ckDetailedBalance->isChecked()) {
    if (m_uiForm.spDetailedBalance->value() == 0.0) {
      uiv.addErrorMessage("Detailed Balance must be more than 0K");
    }
  }

  // Spectra Number check
  const int specMin = m_uiForm.spSpectraMin->value();
  const int specMax = m_uiForm.spSpectraMax->value();
  if (specMin > specMax) {
    uiv.addErrorMessage("Spectra Min must be less than Spectra Max");
  }

  // Background Removal (TOF)
  if (m_uiForm.ckBackgroundRemoval->isChecked()) {
    const int start = m_uiForm.spBackgroundStart->value();
    const int end = m_uiForm.spBackgroundEnd->value();
    if (start > end) {
      uiv.addErrorMessage("Background Start must be less than Background End");
    }
  }

  if (m_uiForm.dsRunFiles->isValid()) {
    int detectorMin = m_uiForm.spPlotTimeSpecMin->value();
    int detectorMax = m_uiForm.spPlotTimeSpecMax->value();

    QString rawFile = m_uiForm.dsRunFiles->getFirstFilename();
    auto pos = rawFile.lastIndexOf(".");
    auto extension = rawFile.right(rawFile.length() - pos);
    QFileInfo rawFileInfo(rawFile);
    std::string name = rawFileInfo.baseName().toStdString();

    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    loadAlg->setProperty("Filename", rawFile.toStdString());
    loadAlg->setProperty("OutputWorkspace", name);
    if (extension.compare(".nxs") == 0) {
      int64_t detectorMin =
          static_cast<int64_t>(m_uiForm.spPlotTimeSpecMin->value());
      int64_t detectorMax =
          static_cast<int64_t>(m_uiForm.spPlotTimeSpecMax->value());
      loadAlg->setProperty("SpectrumMin", detectorMin);
      loadAlg->setProperty("SpectrumMax", detectorMax);
    } else {
      loadAlg->setProperty("SpectrumMin", detectorMin);
      loadAlg->setProperty("SpectrumMax", detectorMax);
    }

    loadAlg->execute();

    if (m_uiForm.ckBackgroundRemoval->isChecked()) {
      MatrixWorkspace_sptr tempWs =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
      const double minBack = tempWs->x(0)[0];
      const double maxBack = tempWs->x(0)[tempWs->blocksize()];

      if (m_uiForm.spBackgroundStart->value() < minBack) {
        uiv.addErrorMessage("The Start of Background Removal is less than the "
                            "minimum of the data range");
      }

      if (m_uiForm.spBackgroundEnd->value() > maxBack) {
        uiv.addErrorMessage("The End of Background Removal is more than the "
                            "maximum of the data range");
      }
    }
  }

  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  return uiv.isAllInputValid();
}

bool ISISEnergyTransfer::numberInCorrectRange(
    std::size_t const &spectraNumber) const {
  QMap<QString, QString> instDetails = getInstrumentDetails();
  auto spectraMin =
      static_cast<std::size_t>(instDetails["spectra-min"].toInt());
  auto spectraMax =
      static_cast<std::size_t>(instDetails["spectra-max"].toInt());
  return spectraNumber >= spectraMin && spectraNumber <= spectraMax;
}

QString ISISEnergyTransfer::checkCustomGroupingNumbersInRange(
    std::vector<std::size_t> const &customGroupingNumbers) const {
  for (const auto &number : customGroupingNumbers)
    if (!numberInCorrectRange(number))
      return "Please supply a custom grouping within the correct range";
  return "";
}

QString ISISEnergyTransfer::validateDetectorGrouping() const {
  if (m_uiForm.cbGroupingOptions->currentText() == "File") {
    if (!m_uiForm.dsMapFile->isValid())
      return "Mapping file is invalid.";
  } else if (m_uiForm.cbGroupingOptions->currentText() == "Custom") {
    const std::string customString =
        m_uiForm.leCustomGroups->text().toStdString();
    if (customString.empty())
      return "Please supply a custom grouping for detectors.";
    else
      return checkCustomGroupingNumbersInRange(
          getCustomGroupingNumbers(customString));
  }
  return "";
}

void ISISEnergyTransfer::run() {
  IAlgorithm_sptr reductionAlg =
      AlgorithmManager::Instance().create("ISISIndirectEnergyTransfer");
  reductionAlg->initialize();
  BatchAlgorithmRunner::AlgorithmRuntimeProps reductionRuntimeProps;

  QString instName = getInstrumentConfiguration()->getInstrumentName();

  reductionAlg->setProperty("Instrument", instName.toStdString());
  reductionAlg->setProperty(
      "Analyser",
      getInstrumentConfiguration()->getAnalyserName().toStdString());
  reductionAlg->setProperty(
      "Reflection",
      getInstrumentConfiguration()->getReflectionName().toStdString());

  // Override the efixed for QENS spectrometers only
  QStringList qens;
  qens << "IRIS"
       << "OSIRIS";
  if (qens.contains(instName))
    reductionAlg->setProperty("Efixed", m_uiForm.spEfixed->value());

  QString files = m_uiForm.dsRunFiles->getFilenames().join(",");
  reductionAlg->setProperty("InputFiles", files.toStdString());

  reductionAlg->setProperty("SumFiles", m_uiForm.ckSumFiles->isChecked());
  reductionAlg->setProperty("LoadLogFiles",
                            m_uiForm.ckLoadLogFiles->isChecked());

  if (m_uiForm.ckUseCalib->isChecked()) {
    QString calibWorkspaceName =
        m_uiForm.dsCalibrationFile->getCurrentDataName();
    reductionAlg->setProperty("CalibrationWorkspace",
                              calibWorkspaceName.toStdString());
  }

  std::vector<long> detectorRange;
  detectorRange.push_back(m_uiForm.spSpectraMin->value());
  detectorRange.push_back(m_uiForm.spSpectraMax->value());
  reductionAlg->setProperty("SpectraRange", detectorRange);

  if (m_uiForm.ckBackgroundRemoval->isChecked()) {
    std::vector<double> backgroundRange;
    backgroundRange.push_back(m_uiForm.spBackgroundStart->value());
    backgroundRange.push_back(m_uiForm.spBackgroundEnd->value());
    reductionAlg->setProperty("BackgroundRange", backgroundRange);
  }

  if (!m_uiForm.ckDoNotRebin->isChecked()) {
    QString rebin;
    if (m_uiForm.cbRebinType->currentIndex() == 0)
      rebin = m_uiForm.spRebinLow->text() + "," +
              m_uiForm.spRebinWidth->text() + "," +
              m_uiForm.spRebinHigh->text();
    else
      rebin = m_uiForm.leRebinString->text();

    reductionAlg->setProperty("RebinString", rebin.toStdString());
  }

  if (m_uiForm.ckDetailedBalance->isChecked())
    reductionAlg->setProperty("DetailedBalance",
                              m_uiForm.spDetailedBalance->value());

  if (m_uiForm.ckScaleMultiplier->isChecked())
    reductionAlg->setProperty("ScaleFactor",
                              m_uiForm.spScaleMultiplier->value());

  if (m_uiForm.ckCm1Units->isChecked())
    reductionAlg->setProperty("UnitX", "DeltaE_inWavenumber");

  std::pair<std::string, std::string> grouping =
      createMapFile(m_uiForm.cbGroupingOptions->currentText().toStdString());

  reductionAlg->setProperty("GroupingMethod", grouping.first);

  if (grouping.first == "File")
    reductionAlg->setProperty("MapFile", grouping.second);
  else if (grouping.first == "Custom")
    reductionAlg->setProperty("GroupingString", grouping.second);

  reductionAlg->setProperty("FoldMultipleFrames", m_uiForm.ckFold->isChecked());
  reductionAlg->setProperty("OutputWorkspace",
                            "IndirectEnergyTransfer_Workspaces");

  m_batchAlgoRunner->addAlgorithm(reductionAlg, reductionRuntimeProps);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(plotRawComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the algorithm.
 *
 * Sets result workspace for Python export and ungroups result WorkspaceGroup.
 *
 * @param error True if the algorithm was stopped due to error, false otherwise
 */
void ISISEnergyTransfer::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error)
    return;

  WorkspaceGroup_sptr energyTransferOutputGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          "IndirectEnergyTransfer_Workspaces");
  if (energyTransferOutputGroup->size() == 0)
    return;

  // Set workspace for Python export as the first result workspace
  m_pythonExportWsName = energyTransferOutputGroup->getNames()[0];
  m_outputWorkspaces = energyTransferOutputGroup->getNames();
  // Ungroup the output workspace
  energyTransferOutputGroup->removeAll();
  AnalysisDataService::Instance().remove("IndirectEnergyTransfer_Workspaces");

  // Enable plotting and saving
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.cbPlotType->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
  m_uiForm.ckSaveAclimax->setEnabled(true);
  m_uiForm.ckSaveASCII->setEnabled(true);
  m_uiForm.ckSaveDaveGrp->setEnabled(true);
  m_uiForm.ckSaveNexus->setEnabled(true);
  m_uiForm.ckSaveNXSPE->setEnabled(true);
  m_uiForm.ckSaveSPE->setEnabled(true);
}

int ISISEnergyTransfer::getGroupingOptionIndex(QString const &option) {
  for (auto i = 0; i < m_uiForm.cbGroupingOptions->count(); ++i)
    if (m_uiForm.cbGroupingOptions->itemText(i) == option)
      return i;
  return 0;
}

bool ISISEnergyTransfer::isOptionHidden(QString const &option) {
  for (auto i = 0; i < m_uiForm.cbGroupingOptions->count(); ++i)
    if (m_uiForm.cbGroupingOptions->itemText(i) == option)
      return false;
  return true;
}

void ISISEnergyTransfer::setCurrentGroupingOption(QString const &option) {
  m_uiForm.cbGroupingOptions->setCurrentIndex(getGroupingOptionIndex(option));
}

void ISISEnergyTransfer::removeGroupingOption(QString const &option) {
  m_uiForm.cbGroupingOptions->removeItem(getGroupingOptionIndex(option));
}

void ISISEnergyTransfer::includeExtraGroupingOption(bool includeOption,
                                                    QString const &option) {
  if (includeOption && isOptionHidden(option)) {
    m_uiForm.cbGroupingOptions->addItem(option);
    setCurrentGroupingOption(option);
  } else if (!includeOption && !isOptionHidden(option))
    removeGroupingOption(option);
}

/**
 * Called when the instrument has changed, used to update default values.
 */
void ISISEnergyTransfer::setInstrumentDefault() {
  QMap<QString, QString> instDetails = getInstrumentDetails();

  // Set the search instrument for runs
  m_uiForm.dsRunFiles->setInstrumentOverride(instDetails["instrument"]);

  QStringList qens;
  qens << "IRIS"
       << "OSIRIS";
  m_uiForm.spEfixed->setEnabled(qens.contains(instDetails["instrument"]));

  QStringList allowDefaultGroupingInstruments;
  allowDefaultGroupingInstruments << "TOSCA";
  includeExtraGroupingOption(
      allowDefaultGroupingInstruments.contains(instDetails["instrument"]),
      "Default");

  if (instDetails["spectra-min"].isEmpty() ||
      instDetails["spectra-max"].isEmpty()) {
    emit showMessageBox("Could not gather necessary data from parameter file.");
    return;
  }

  // Set spectra min/max for spinners in UI
  const int specMin = instDetails["spectra-min"].toInt();
  const int specMax = instDetails["spectra-max"].toInt();
  // Spectra spinners
  m_uiForm.spSpectraMin->setMinimum(specMin);
  m_uiForm.spSpectraMin->setMaximum(specMax);
  m_uiForm.spSpectraMin->setValue(specMin);

  m_uiForm.spSpectraMax->setMinimum(specMin);
  m_uiForm.spSpectraMax->setMaximum(specMax);
  m_uiForm.spSpectraMax->setValue(specMax);

  // Plot time spectra spinners
  m_uiForm.spPlotTimeSpecMin->setMinimum(1); // 1 to allow for monitors
  m_uiForm.spPlotTimeSpecMin->setMaximum(specMax);
  m_uiForm.spPlotTimeSpecMin->setValue(1);

  m_uiForm.spPlotTimeSpecMax->setMinimum(1);
  m_uiForm.spPlotTimeSpecMax->setMaximum(specMax);
  m_uiForm.spPlotTimeSpecMax->setValue(1);

  if (!instDetails["Efixed"].isEmpty())
    m_uiForm.spEfixed->setValue(instDetails["Efixed"].toDouble());
  else
    m_uiForm.spEfixed->setValue(0.0);

  // Default rebinning parameters can be set in instrument parameter file
  if (!instDetails["rebin-default"].isEmpty()) {
    m_uiForm.leRebinString->setText(instDetails["rebin-default"]);
    m_uiForm.ckDoNotRebin->setChecked(false);
    QStringList rbp =
        instDetails["rebin-default"].split(",", QString::SkipEmptyParts);
    if (rbp.size() == 3) {
      m_uiForm.spRebinLow->setValue(rbp[0].toDouble());
      m_uiForm.spRebinWidth->setValue(rbp[1].toDouble());
      m_uiForm.spRebinHigh->setValue(rbp[2].toDouble());
      m_uiForm.cbRebinType->setCurrentIndex(0);
    } else {
      m_uiForm.cbRebinType->setCurrentIndex(1);
    }
  } else {
    m_uiForm.ckDoNotRebin->setChecked(true);
    m_uiForm.spRebinLow->setValue(0.0);
    m_uiForm.spRebinWidth->setValue(0.0);
    m_uiForm.spRebinHigh->setValue(0.0);
    m_uiForm.leRebinString->setText("");
  }

  if (!instDetails["cm-1-convert-choice"].isEmpty()) {
    bool defaultOptions = instDetails["cm-1-convert-choice"] == "true";
    m_uiForm.ckCm1Units->setChecked(defaultOptions);
  }

  if (!instDetails["save-nexus-choice"].isEmpty()) {
    bool defaultOptions = instDetails["save-nexus-choice"] == "true";
    m_uiForm.ckSaveNexus->setChecked(defaultOptions);
  }

  if (!instDetails["save-ascii-choice"].isEmpty()) {
    bool defaultOptions = instDetails["save-ascii-choice"] == "true";
    m_uiForm.ckSaveASCII->setChecked(defaultOptions);
  }

  if (!instDetails["fold-frames-choice"].isEmpty()) {
    bool defaultOptions = instDetails["fold-frames-choice"] == "true";
    m_uiForm.ckFold->setChecked(defaultOptions);
  }
}

/**
 * This function runs when the user makes a selection on the cbGroupingOptions
 * QComboBox.
 * @param groupType :: Value of selection made by user.
 */
void ISISEnergyTransfer::mappingOptionSelected(const QString &groupType) {
  if (groupType == "File")
    m_uiForm.swGrouping->setCurrentIndex(0);
  else if (groupType == "Groups")
    m_uiForm.swGrouping->setCurrentIndex(1);
  else if (groupType == "Custom")
    m_uiForm.swGrouping->setCurrentIndex(2);
  else
    m_uiForm.swGrouping->setCurrentIndex(3);
}

/**
 * This function creates the mapping/grouping file for the data analysis.
 * @param groupType :: Type of grouping (All, Group, Indiviual)
 * @return path to mapping file, or an empty string if file could not be
 * created.
 */
std::pair<std::string, std::string>
ISISEnergyTransfer::createMapFile(const std::string &groupType) {
  QString specRange =
      m_uiForm.spSpectraMin->text() + "," + m_uiForm.spSpectraMax->text();

  if (groupType == "File") {
    QString groupFile = m_uiForm.dsMapFile->getFirstFilename();
    if (groupFile == "")
      emit showMessageBox("You must enter a path to the .map file.");

    return std::make_pair("File", groupFile.toStdString());
  } else if (groupType == "Groups")
    return std::make_pair("Custom", getDetectorGroupingString());
  else if (groupType == "Default")
    return std::make_pair("IPF", "");
  else if (groupType == "Custom")
    return std::make_pair("Custom",
                          m_uiForm.leCustomGroups->text().toStdString());
  else {
    // Catch All and Individual
    return std::make_pair(groupType, "");
  }
}

std::string ISISEnergyTransfer::getDetectorGroupingString() const {
  const unsigned int nGroups = m_uiForm.spNumberGroups->value();
  const unsigned int nSpectra =
      1 + m_uiForm.spSpectraMax->value() - m_uiForm.spSpectraMin->value();
  return createDetectorGroupingString(static_cast<std::size_t>(nSpectra),
                                      static_cast<std::size_t>(nGroups));
}

/**
 * Converts the checkbox selection to a comma delimited list of save formats for
 *the
 * ISISIndirectEnergyTransfer algorithm.
 *
 * @return A vector of save formats
 */
std::vector<std::string> ISISEnergyTransfer::getSaveFormats() {
  std::vector<std::string> fileFormats;

  if (m_uiForm.ckSaveNexus->isChecked())
    fileFormats.emplace_back("nxs");
  if (m_uiForm.ckSaveSPE->isChecked())
    fileFormats.emplace_back("spe");
  if (m_uiForm.ckSaveNXSPE->isChecked())
    fileFormats.emplace_back("nxspe");
  if (m_uiForm.ckSaveASCII->isChecked())
    fileFormats.emplace_back("ascii");
  if (m_uiForm.ckSaveAclimax->isChecked())
    fileFormats.emplace_back("aclimax");
  if (m_uiForm.ckSaveDaveGrp->isChecked())
    fileFormats.emplace_back("davegrp");

  return fileFormats;
}

/**
 * Plots raw time data from .raw file before any data conversion has been
 * performed.
 */
void ISISEnergyTransfer::plotRaw() {
  using Mantid::specnum_t;
  using MantidQt::API::BatchAlgorithmRunner;

  if (!m_uiForm.dsRunFiles->isValid()) {
    emit showMessageBox("You must select a run file.");
    return;
  }

  int detectorMin = m_uiForm.spPlotTimeSpecMin->value();
  int detectorMax = m_uiForm.spPlotTimeSpecMax->value();

  if (detectorMin > detectorMax) {
    emit showMessageBox(
        "Minimum spectra must be less than or equal to maximum spectra.");
    return;
  }
  const int startBack = m_uiForm.spBackgroundStart->value();
  const int endBack = m_uiForm.spBackgroundEnd->value();

  if (m_uiForm.ckBackgroundRemoval->isChecked() == true) {
    if (startBack > endBack) {
      emit showMessageBox("Background Start must be less than Background End");
      return;
    }
  }

  QString rawFile = m_uiForm.dsRunFiles->getFirstFilename();
  auto pos = rawFile.lastIndexOf(".");
  auto extension = rawFile.right(rawFile.length() - pos);
  QFileInfo rawFileInfo(rawFile);
  std::string name = rawFileInfo.baseName().toStdString();

  IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
  loadAlg->initialize();
  loadAlg->setProperty("Filename", rawFile.toStdString());
  loadAlg->setProperty("OutputWorkspace", name);
  loadAlg->setProperty("LoadLogFiles", false);
  if (extension.compare(".nxs") == 0) {
    int64_t detectorMin =
        static_cast<int64_t>(m_uiForm.spPlotTimeSpecMin->value());
    int64_t detectorMax =
        static_cast<int64_t>(m_uiForm.spPlotTimeSpecMax->value());
    loadAlg->setProperty("SpectrumMin", detectorMin);
    loadAlg->setProperty("SpectrumMax", detectorMax);
  } else {
    loadAlg->setProperty("SpectrumMin", detectorMin);
    loadAlg->setProperty("SpectrumMax", detectorMax);
  }

  loadAlg->execute();

  if (m_uiForm.ckBackgroundRemoval->isChecked()) {
    MatrixWorkspace_sptr tempWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);

    const double minBack = tempWs->x(0)[0];
    const double maxBack = tempWs->x(0)[tempWs->blocksize()];

    if (startBack < minBack) {
      emit showMessageBox("The Start of Background Removal is less than the "
                          "minimum of the data range");
      return;
    }

    if (endBack > maxBack) {
      emit showMessageBox("The End of Background Removal is more than the "
                          "maximum of the data range");
      return;
    }
  }

  // Rebin the workspace to its self to ensure constant binning
  BatchAlgorithmRunner::AlgorithmRuntimeProps inputToRebin;
  inputToRebin["WorkspaceToMatch"] = name;
  inputToRebin["WorkspaceToRebin"] = name;
  inputToRebin["OutputWorkspace"] = name;

  IAlgorithm_sptr rebinAlg =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebinAlg->initialize();
  m_batchAlgoRunner->addAlgorithm(rebinAlg, inputToRebin);

  BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromRebin;
  inputFromRebin["InputWorkspace"] = name;

  std::vector<specnum_t> detectorList;
  for (specnum_t i = detectorMin; i <= detectorMax; i++)
    detectorList.push_back(i);

  if (m_uiForm.ckBackgroundRemoval->isChecked()) {
    std::vector<double> range;
    range.push_back(m_uiForm.spBackgroundStart->value());
    range.push_back(m_uiForm.spBackgroundEnd->value());

    IAlgorithm_sptr calcBackAlg =
        AlgorithmManager::Instance().create("CalculateFlatBackground");
    calcBackAlg->initialize();
    calcBackAlg->setProperty("OutputWorkspace", name + "_bg");
    calcBackAlg->setProperty("Mode", "Mean");
    calcBackAlg->setProperty("StartX", range[0]);
    calcBackAlg->setProperty("EndX", range[1]);
    m_batchAlgoRunner->addAlgorithm(calcBackAlg, inputFromRebin);

    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromCalcBG;
    inputFromCalcBG["InputWorkspace"] = name + "_bg";

    IAlgorithm_sptr groupAlg =
        AlgorithmManager::Instance().create("GroupDetectors");
    groupAlg->initialize();
    groupAlg->setProperty("OutputWorkspace", name + "_grp");
    groupAlg->setProperty("DetectorList", detectorList);
    m_batchAlgoRunner->addAlgorithm(groupAlg, inputFromCalcBG);

    IAlgorithm_sptr rawGroupAlg =
        AlgorithmManager::Instance().create("GroupDetectors");
    rawGroupAlg->initialize();
    rawGroupAlg->setProperty("OutputWorkspace", name + "_grp_raw");
    rawGroupAlg->setProperty("DetectorList", detectorList);
    m_batchAlgoRunner->addAlgorithm(rawGroupAlg, inputFromRebin);
  } else {
    IAlgorithm_sptr rawGroupAlg =
        AlgorithmManager::Instance().create("GroupDetectors");
    rawGroupAlg->initialize();
    rawGroupAlg->setProperty("OutputWorkspace", name + "_grp");
    rawGroupAlg->setProperty("DetectorList", detectorList);
    m_batchAlgoRunner->addAlgorithm(rawGroupAlg, inputFromRebin);
  }

  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(plotRawComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles plotting the result of Plot Raw
 *
 * @param error Indicates if the algorithm chain failed
 */
void ISISEnergyTransfer::plotRawComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(plotRawComplete(bool)));

  if (error)
    return;

  QString rawFile = m_uiForm.dsRunFiles->getFirstFilename();
  QFileInfo rawFileInfo(rawFile);
  std::string name = rawFileInfo.baseName().toStdString();

  plotSpectrum(QString::fromStdString(name) + "_grp");
}

/**
 * Called when a user starts to type / edit the runs to load.
 */
void ISISEnergyTransfer::pbRunEditing() {
  emit updateRunButton(false, "Editing...",
                       "Run numbers are currently being edited.");
}

/**
 * Called when the FileFinder starts finding the files.
 */
void ISISEnergyTransfer::pbRunFinding() {
  emit updateRunButton(
      false, "Finding files...",
      "Searching for data files for the run numbers entered...");
  m_uiForm.dsRunFiles->setEnabled(false);
}

/**
 * Called when the FileFinder has finished finding the files.
 */
void ISISEnergyTransfer::pbRunFinished() {
  if (!m_uiForm.dsRunFiles->isValid()) {
    emit updateRunButton(
        false, "Invalid Run(s)",
        "Cannot find data files for some of the run numbers entered.");
  } else {
    emit updateRunButton();
  }

  m_uiForm.dsRunFiles->setEnabled(true);
}
/**
 * Handle mantid plotting of workspaces
 */
void ISISEnergyTransfer::plotClicked() {
  for (const auto &it : m_outputWorkspaces) {
    if (checkADSForPlotSaveWorkspace(it, true)) {
      const auto plotType = m_uiForm.cbPlotType->currentText();
      QString pyInput = "from IndirectReductionCommon import plot_reduction\n";
      pyInput += "plot_reduction('";
      pyInput += QString::fromStdString(it) + "', '";
      pyInput += plotType + "')\n";
      m_pythonRunner.runPythonCode(pyInput);
    }
  }
}

/**
 * Handle saving of workspaces
 */
void ISISEnergyTransfer::saveClicked() {
  auto saveFormats = getSaveFormats();
  QString pyInput = "from IndirectReductionCommon import save_reduction\n";
  pyInput += "save_reduction([";
  for (const auto &it : m_outputWorkspaces) {
    pyInput += "'" + QString::fromStdString(it) + "', ";
  }
  pyInput += "], [";
  for (const auto &it : saveFormats) {
    pyInput += "'" + QString::fromStdString(it) + "', ";
  }
  pyInput += "]";
  if (m_uiForm.ckCm1Units->isChecked())
    pyInput += ", 'DeltaE_inWavenumber'";
  pyInput += ")\n";
  m_pythonRunner.runPythonCode(pyInput);
}

} // namespace CustomInterfaces
} // namespace MantidQt
