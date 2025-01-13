// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentPresenter.h"
#include "Common/Parse.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "LookupTableValidator.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/ParseReflectometryStrings.h"
#include "Reduction/PreviewRow.h"
#include "Reduction/RowExceptions.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {
// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");
}

ExperimentPresenter::ExperimentPresenter(IExperimentView *view, Experiment experiment, double defaultsThetaTolerance,
                                         IFileHandler *fileHandler,
                                         std::unique_ptr<IExperimentOptionDefaults> experimentDefaults)
    : m_experimentDefaults(std::move(experimentDefaults)), m_view(view), m_fileHandler(fileHandler),
      m_model(std::move(experiment)), m_thetaTolerance(defaultsThetaTolerance), m_validationResult(m_model) {
  m_view->subscribe(this);
}

void ExperimentPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

Experiment const &ExperimentPresenter::experiment() const { return m_model; }

void ExperimentPresenter::notifySettingsChanged() {
  updateModelFromView();
  showValidationResult();
  m_mainPresenter->notifySettingsChanged();
}

void ExperimentPresenter::notifyRestoreDefaultsRequested() {
  // Trigger a reload of the instrument to get up-to-date settings.
  m_mainPresenter->notifyUpdateInstrumentRequested();
  restoreDefaults();
}

void ExperimentPresenter::notifySummationTypeChanged() { notifySettingsChanged(); }

void ExperimentPresenter::updateSummationTypeEnabledState() {
  if (m_model.summationType() == SummationType::SumInQ) {
    m_view->enableReductionType();
    m_view->enableIncludePartialBins();
  } else {
    m_view->disableReductionType();
    m_view->disableIncludePartialBins();
  }
}

void ExperimentPresenter::notifyNewLookupRowRequested() {
  m_view->addLookupRow();
  notifySettingsChanged();
}

void ExperimentPresenter::notifyRemoveLookupRowRequested(int index) {
  m_view->removeLookupRow(index);
  notifySettingsChanged();
}

void ExperimentPresenter::notifyLookupRowChanged(int /*row*/, int /*column*/) {
  updateModelFromView();
  showValidationResult();
  m_mainPresenter->notifySettingsChanged();
}

bool ExperimentPresenter::isProcessing() const { return m_mainPresenter->isProcessing(); }

bool ExperimentPresenter::isAutoreducing() const { return m_mainPresenter->isAutoreducing(); }

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void ExperimentPresenter::updateWidgetEnabledState() {
  if (isProcessing() || isAutoreducing()) {
    m_view->disableAll();
    return;
  }

  m_view->enableAll();
  updateSummationTypeEnabledState();
  updateBackgroundSubtractionEnabledState();
  updatePolarizationCorrectionEnabledState();
  updateFloodCorrectionEnabledState();
}

void ExperimentPresenter::notifyReductionPaused() { updateWidgetEnabledState(); }

void ExperimentPresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void ExperimentPresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void ExperimentPresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

void ExperimentPresenter::notifyInstrumentChanged(std::string const &instrumentName) {
  UNUSED_ARG(instrumentName);
  restoreDefaults();
}

namespace {
bool hasUpdatedSettings(boost::optional<LookupRow> const lookupRow, PreviewRow const &previewRow) {
  return lookupRow->roiDetectorIDs() != previewRow.getSelectedBanks() ||
         lookupRow->processingInstructions() != previewRow.getProcessingInstructions(ROIType::Signal) ||
         lookupRow->backgroundProcessingInstructions() != previewRow.getProcessingInstructions(ROIType::Background) ||
         lookupRow->transmissionProcessingInstructions() != previewRow.getProcessingInstructions(ROIType::Transmission);
}
} // namespace

void ExperimentPresenter::notifyPreviewApplyRequested(PreviewRow const &previewRow) {
  if (!hasValidSettings()) {
    throw InvalidTableException("The Experiment Settings table contains invalid settings.");
  }
  if (auto const foundRow = m_model.findLookupRow(previewRow, m_thetaTolerance)) {
    if (!hasUpdatedSettings(foundRow, previewRow)) {
      return;
    }

    auto lookupRowCopy = *foundRow;

    lookupRowCopy.setRoiDetectorIDs(previewRow.getSelectedBanks());

    updateLookupRowProcessingInstructions(previewRow, lookupRowCopy, ROIType::Signal);
    updateLookupRowProcessingInstructions(previewRow, lookupRowCopy, ROIType::Background);
    updateLookupRowProcessingInstructions(previewRow, lookupRowCopy, ROIType::Transmission);

    m_model.updateLookupRow(std::move(lookupRowCopy), m_thetaTolerance);
    updateViewFromModel();
    m_mainPresenter->notifySettingsChanged();
  } else {
    throw RowNotFoundException("There is no row with angle matching '" + std::to_string(previewRow.theta()) +
                               "' in the Lookup Table.");
  }
}

void ExperimentPresenter::updateLookupRowProcessingInstructions(PreviewRow const &previewRow, LookupRow &lookupRow,
                                                                ROIType regionType) {
  auto const instructions = previewRow.getProcessingInstructions(regionType);
  lookupRow.setProcessingInstructions(regionType, std::move(instructions));
}

void ExperimentPresenter::restoreDefaults() {
  auto const instrument = m_mainPresenter->instrument();
  try {
    m_model = m_experimentDefaults->get(instrument);
  } catch (std::invalid_argument &ex) {
    std::ostringstream msg;
    msg << "Error setting default Experiment Settings: " << ex.what() << ". Please check the " << instrument->getName()
        << " parameters file.";
    g_log.error(msg.str());
    m_model = Experiment();
  }
  updateViewFromModel();
}

BackgroundSubtraction ExperimentPresenter::backgroundSubtractionFromView() {
  auto const subtractBackground = m_view->getSubtractBackground();
  auto const subtractionType = backgroundSubtractionTypeFromString(m_view->getBackgroundSubtractionMethod());
  auto const degreeOfPolynomial = m_view->getPolynomialDegree();
  auto const costFunction = costFunctionTypeFromString(m_view->getCostFunction());
  return BackgroundSubtraction(subtractBackground, subtractionType, degreeOfPolynomial, costFunction);
}

PolarizationCorrections ExperimentPresenter::polarizationCorrectionsFromView() {
  auto const &polCorrOptionString = m_view->getPolarizationCorrectionOption();
  auto const &polCorrType = polarizationCorrectionTypeFromString(polCorrOptionString);

  if (polCorrType == PolarizationCorrectionType::None || polCorrType == PolarizationCorrectionType::ParameterFile) {
    return PolarizationCorrections(polCorrType);
  }
  if (polCorrOptionString == "FilePath") {
    auto const &polCorrFilePath = m_view->getPolarizationEfficienciesFilePath();
    showPolCorrFilePathValidity(polCorrFilePath);
    return PolarizationCorrections(polCorrType, polCorrFilePath);
  }
  return PolarizationCorrections(polCorrType, m_view->getPolarizationEfficienciesWorkspace());
}

void ExperimentPresenter::showPolCorrFilePathValidity(std::string const &filePath) {
  if (m_fileHandler->fileExists(m_fileHandler->getFullFilePath(filePath))) {
    m_view->showPolCorrFilePathValid();
    return;
  }
  m_view->showPolCorrFilePathInvalid();
}

FloodCorrections ExperimentPresenter::floodCorrectionsFromView() {
  auto const &correctionTypeString = m_view->getFloodCorrectionType();
  auto const &correctionType = floodCorrectionTypeFromString(correctionTypeString);

  if (floodCorrectionRequiresInputs(correctionType)) {
    if (correctionTypeString == "Workspace") {
      return FloodCorrections(correctionType, m_view->getFloodWorkspace());
    }
    if (correctionTypeString == "FilePath") {
      auto const &floodFilePath = m_view->getFloodFilePath();
      showFloodFilePathValidity(floodFilePath);
      return FloodCorrections(correctionType, floodFilePath);
    }
  }
  return FloodCorrections(correctionType);
}

void ExperimentPresenter::showFloodFilePathValidity(const std::string &filePath) {
  if (m_fileHandler->fileExists(m_fileHandler->getFullFilePath(filePath))) {
    m_view->showFloodCorrFilePathValid();
    return;
  }
  m_view->showFloodCorrFilePathInvalid();
}

void ExperimentPresenter::updateBackgroundSubtractionEnabledState() {
  if (m_view->getSubtractBackground()) {
    m_view->enableBackgroundSubtractionMethod();
    if (m_view->getBackgroundSubtractionMethod() == "Polynomial") {
      m_view->enablePolynomialDegree();
      m_view->enableCostFunction();
    } else {
      m_view->disablePolynomialDegree();
      m_view->disableCostFunction();
    }
  } else {
    m_view->disableBackgroundSubtractionMethod();
    m_view->disablePolynomialDegree();
    m_view->disableCostFunction();
  }
}

void ExperimentPresenter::updatePolarizationCorrectionEnabledState() {
  // We could generalise which instruments polarization corrections are
  // applicable for but for now it's not worth it, so just hard code the
  // instrument names.
  auto const &instrumentName = m_mainPresenter->instrumentName();
  if (instrumentName == "INTER" || instrumentName == "SURF") {
    m_view->setPolarizationCorrectionOption("None");
    m_view->disablePolarizationCorrections();
    disablePolarizationEfficiencies();
    return;
  }
  auto const &polCorrOption = m_view->getPolarizationCorrectionOption();
  m_view->enablePolarizationCorrections();
  if (polCorrOption == "ParameterFile" || polCorrOption == "None") {
    disablePolarizationEfficiencies();
    return;
  }
  if (polCorrOption == "Workspace") {
    m_view->enablePolarizationEfficiencies();
    m_view->setPolarizationEfficienciesWorkspaceMode();
    return;
  }
  if (polCorrOption == "FilePath") {
    m_view->enablePolarizationEfficiencies();
    m_view->setPolarizationEfficienciesFilePathMode();
    return;
  }
}

void ExperimentPresenter::disablePolarizationEfficiencies() {
  m_view->setPolarizationEfficienciesWorkspaceMode();
  m_view->disablePolarizationEfficiencies();
}

void ExperimentPresenter::updateFloodCorrectionEnabledState() {
  auto const &floodCorrOption = m_view->getFloodCorrectionType();

  if (floodCorrOption == "None" || floodCorrOption == "ParameterFile") {
    disableFloodCorrectionInputs();
    return;
  }
  if (floodCorrOption == "Workspace") {
    m_view->enableFloodCorrectionInputs();
    m_view->setFloodCorrectionWorkspaceMode();
    return;
  }
  if (floodCorrOption == "FilePath") {
    m_view->enableFloodCorrectionInputs();
    m_view->setFloodCorrectionFilePathMode();
    return;
  }
}

void ExperimentPresenter::disableFloodCorrectionInputs() {
  m_view->setFloodCorrectionWorkspaceMode();
  m_view->disableFloodCorrectionInputs();
}

std::optional<RangeInLambda> ExperimentPresenter::transmissionRunRangeFromView() {
  auto const range = RangeInLambda(m_view->getTransmissionStartOverlap(), m_view->getTransmissionEndOverlap());
  auto const bothOrNoneMustBeSet = false;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showTransmissionRangeValid();
  else
    m_view->showTransmissionRangeInvalid();

  if (range.unset() || !range.isValid(bothOrNoneMustBeSet))
    return std::nullopt;
  else
    return range;
}

std::string ExperimentPresenter::transmissionStitchParamsFromView() {
  auto stitchParams = m_view->getTransmissionStitchParams();
  // It's valid if empty
  if (stitchParams.empty()) {
    m_view->showTransmissionStitchParamsValid();
    return stitchParams;
  }

  // If set, the params should be a list containing an odd number of double
  // values (as per the Params property of Rebin)
  auto maybeParamsList = parseList(stitchParams, parseDouble);
  if (maybeParamsList.has_value() && maybeParamsList->size() % 2 != 0) {
    m_view->showTransmissionStitchParamsValid();
    return stitchParams;
  }

  m_view->showTransmissionStitchParamsInvalid();
  return std::string();
}

TransmissionStitchOptions ExperimentPresenter::transmissionStitchOptionsFromView() {
  auto transmissionRunRange = transmissionRunRangeFromView();
  auto stitchParams = transmissionStitchParamsFromView();
  auto scaleRHS = m_view->getTransmissionScaleRHSWorkspace();
  return TransmissionStitchOptions(transmissionRunRange, stitchParams, scaleRHS);
}

std::map<std::string, std::string> ExperimentPresenter::stitchParametersFromView() {
  auto maybeStitchParameters = parseOptions(m_view->getStitchOptions());
  if (maybeStitchParameters.is_initialized()) {
    m_view->showStitchParametersValid();
    return maybeStitchParameters.get();
  }

  m_view->showStitchParametersInvalid();
  return std::map<std::string, std::string>();
}

bool ExperimentPresenter::hasValidSettings() const noexcept { return m_validationResult.isValid(); }

ExperimentValidationResult ExperimentPresenter::validateExperimentFromView() {
  auto validate = LookupTableValidator();
  auto lookupTableValidationResult = validate(m_view->getLookupTable(), m_thetaTolerance);
  if (lookupTableValidationResult.isValid()) {
    auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
    auto const reductionType = reductionTypeFromString(m_view->getReductionType());
    auto const summationType = summationTypeFromString(m_view->getSummationType());
    auto const includePartialBins = m_view->getIncludePartialBins();
    auto const debugOption = m_view->getDebugOption();
    auto transmissionStitchOptions = transmissionStitchOptionsFromView();
    auto backgroundSubtraction = backgroundSubtractionFromView();
    auto polarizationCorrections = polarizationCorrectionsFromView();
    auto floodCorrections = floodCorrectionsFromView();
    auto stitchParameters = stitchParametersFromView();
    return ExperimentValidationResult(Experiment(analysisMode, reductionType, summationType, includePartialBins,
                                                 debugOption, backgroundSubtraction, polarizationCorrections,
                                                 floodCorrections, transmissionStitchOptions, stitchParameters,
                                                 lookupTableValidationResult.assertValid()));
  } else {
    return ExperimentValidationResult(ExperimentValidationErrors(lookupTableValidationResult.assertError()));
  }
}

void ExperimentPresenter::updateModelFromView() {
  m_validationResult = validateExperimentFromView();
  if (m_validationResult.isValid()) {
    m_model = m_validationResult.assertValid();
    updateWidgetEnabledState();
  }
}

void ExperimentPresenter::showLookupTableErrors(LookupTableValidationError const &errors) {
  m_view->showAllLookupRowsAsValid();
  for (auto const &validationError : errors.errors()) {
    for (auto const &column : validationError.invalidColumns()) {
      if (errors.fullTableError()) {
        showFullTableError(errors.fullTableError().get(), validationError.row(), column);
      }
      m_view->showLookupRowAsInvalid(validationError.row(), column);
    }
  }
}

void ExperimentPresenter::showFullTableError(LookupCriteriaError const &tableError, int row, int column) {
  if (tableError == LookupCriteriaError::NonUniqueSearchCriteria)
    m_view->setTooltip(row, column,
                       "Error: Duplicated search criteria. No more than one row may have the same angle and title.");
  if (tableError == LookupCriteriaError::MultipleWildcards)
    m_view->setTooltip(
        row, column,
        "Error: Multiple wildcard rows. Only a single row in the table may have a blank angle and title cell.");
}

void ExperimentPresenter::showValidationResult() {
  if (m_validationResult.isValid()) {
    m_view->showAllLookupRowsAsValid();
  } else {
    auto errors = m_validationResult.assertError();
    showLookupTableErrors(errors.lookupTableValidationErrors());
  }
}

void ExperimentPresenter::updateViewFromModel() {
  // Disconnect notifications about settings updates otherwise we'll end
  // up updating the model from the view after the first change
  m_view->disconnectExperimentSettingsWidgets();

  m_view->setAnalysisMode(analysisModeToString(m_model.analysisMode()));
  m_view->setReductionType(reductionTypeToString(m_model.reductionType()));
  m_view->setSummationType(summationTypeToString(m_model.summationType()));
  m_view->setIncludePartialBins(m_model.includePartialBins());
  m_view->setDebugOption(m_model.debug());
  m_view->setLookupTable(m_model.lookupTableToArray());
  // Transmission
  if (m_model.transmissionStitchOptions().overlapRange()) {
    m_view->setTransmissionStartOverlap(m_model.transmissionStitchOptions().overlapRange()->min());
    m_view->setTransmissionEndOverlap(m_model.transmissionStitchOptions().overlapRange()->max());
  } else {
    m_view->setTransmissionStartOverlap(0.0);
    m_view->setTransmissionEndOverlap(0.0);
  }
  m_view->setTransmissionStitchParams(m_model.transmissionStitchOptions().rebinParameters());
  m_view->setTransmissionScaleRHSWorkspace(m_model.transmissionStitchOptions().scaleRHS());
  // Background subtraction
  m_view->setSubtractBackground(m_model.backgroundSubtraction().subtractBackground());
  m_view->setBackgroundSubtractionMethod(
      backgroundSubtractionTypeToString(m_model.backgroundSubtraction().subtractionType()));
  m_view->setPolynomialDegree(m_model.backgroundSubtraction().degreeOfPolynomial());
  m_view->setCostFunction(costFunctionTypeToString(m_model.backgroundSubtraction().costFunction()));
  // Corrections
  m_view->setPolarizationCorrectionOption(
      polarizationCorrectionTypeToString(m_model.polarizationCorrections().correctionType()));
  m_view->setPolarizationEfficienciesFilePath("");
  if (m_model.polarizationCorrections().workspace())
    m_view->setPolarizationEfficienciesWorkspace(m_model.polarizationCorrections().workspace().get());
  m_view->setFloodCorrectionType(floodCorrectionTypeToString(m_model.floodCorrections().correctionType()));
  if (m_model.floodCorrections().workspace())
    m_view->setFloodWorkspace(m_model.floodCorrections().workspace().value());
  else
    m_view->setFloodWorkspace("");
  m_view->setFloodFilePath("");
  m_view->setStitchOptions(m_model.stitchParametersString());

  // We don't allow invalid config so reset all state to valid
  m_view->showAllLookupRowsAsValid();
  m_view->showTransmissionRangeValid();
  m_view->showStitchParametersValid();

  updateWidgetEnabledState();

  // Reconnect settings change notifications
  m_view->connectExperimentSettingsWidgets();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
