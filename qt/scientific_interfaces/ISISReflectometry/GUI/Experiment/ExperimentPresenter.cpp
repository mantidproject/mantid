// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentPresenter.h"
#include "Common/Parse.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "PerThetaDefaultsTableValidator.h"
#include "Reduction/ParseReflectometryStrings.h"
#include "Reduction/ValidatePerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {
// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");
}

ExperimentPresenter::ExperimentPresenter(
    IExperimentView *view, Experiment experiment, double defaultsThetaTolerance,
    std::unique_ptr<IExperimentOptionDefaults> experimentDefaults)
    : m_experimentDefaults(std::move(experimentDefaults)), m_view(view),
      m_model(std::move(experiment)), m_thetaTolerance(defaultsThetaTolerance) {
  m_view->subscribe(this);
}

void ExperimentPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
  notifySettingsChanged();
}

Experiment const &ExperimentPresenter::experiment() const { return m_model; }

void ExperimentPresenter::notifySettingsChanged() {
  auto validationResult = updateModelFromView();
  showValidationResult(validationResult);
  m_mainPresenter->notifySettingsChanged();
}

void ExperimentPresenter::notifyRestoreDefaultsRequested() {
  // Notify main presenter first to make sure instrument is up to date
  m_mainPresenter->notifyRestoreDefaultsRequested();
  restoreDefaults();
}

void ExperimentPresenter::notifySummationTypeChanged() {
  notifySettingsChanged();
}

void ExperimentPresenter::updateSummationTypeEnabledState() {
  if (m_model.summationType() == SummationType::SumInQ) {
    m_view->enableReductionType();
    m_view->enableIncludePartialBins();
  } else {
    m_view->disableReductionType();
    m_view->disableIncludePartialBins();
  }
}

void ExperimentPresenter::notifyNewPerAngleDefaultsRequested() {
  m_view->addPerThetaDefaultsRow();
  notifySettingsChanged();
}

void ExperimentPresenter::notifyRemovePerAngleDefaultsRequested(int index) {
  m_view->removePerThetaDefaultsRow(index);
  notifySettingsChanged();
}

void ExperimentPresenter::notifyPerAngleDefaultsChanged(int, int column) {
  auto validationResult = updateModelFromView();
  showValidationResult(validationResult);
  if (column == 0 && !validationResult.isValid() &&
      validationResult.assertError()
              .perThetaValidationErrors()
              .fullTableError() == ThetaValuesValidationError::NonUniqueTheta)
    m_view->showPerAngleThetasNonUnique(m_thetaTolerance);
  m_mainPresenter->notifySettingsChanged();
}

bool ExperimentPresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool ExperimentPresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}

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
  updatePolarizationCorrectionEnabledState();
  updateFloodCorrectionEnabledState();
}

void ExperimentPresenter::reductionPaused() { updateWidgetEnabledState(); }

void ExperimentPresenter::reductionResumed() { updateWidgetEnabledState(); }

void ExperimentPresenter::autoreductionPaused() { updateWidgetEnabledState(); }

void ExperimentPresenter::autoreductionResumed() { updateWidgetEnabledState(); }

void ExperimentPresenter::instrumentChanged(std::string const &instrumentName) {
  UNUSED_ARG(instrumentName);
  restoreDefaults();
}

void ExperimentPresenter::restoreDefaults() {
  auto const instrument = m_mainPresenter->instrument();
  try {
    m_model = m_experimentDefaults->get(instrument);
  } catch (std::invalid_argument &ex) {
    std::ostringstream msg;
    msg << "Error setting default Experiment Settings: " << ex.what()
        << ". Please check the " << instrument->getName()
        << " parameters file.";
    g_log.error(msg.str());
    m_model = Experiment();
  }
  updateViewFromModel();
}

PolarizationCorrections ExperimentPresenter::polarizationCorrectionsFromView() {
  auto const correctionType = m_view->getPolarizationCorrectionOption()
                                  ? PolarizationCorrectionType::ParameterFile
                                  : PolarizationCorrectionType::None;
  return PolarizationCorrections(correctionType);
}

FloodCorrections ExperimentPresenter::floodCorrectionsFromView() {
  auto const correctionType =
      floodCorrectionTypeFromString(m_view->getFloodCorrectionType());

  if (floodCorrectionRequiresInputs(correctionType)) {
    return FloodCorrections(correctionType, m_view->getFloodWorkspace());
  }

  return FloodCorrections(correctionType);
}

void ExperimentPresenter::updatePolarizationCorrectionEnabledState() {
  // We could generalise which instruments polarization corrections are
  // applicable for but for now it's not worth it, so just hard code the
  // instrument names.
  auto const instrumentName = m_mainPresenter->instrumentName();
  if (instrumentName == "INTER" || instrumentName == "SURF") {
    m_view->setPolarizationCorrectionOption(false);
    m_view->disablePolarizationCorrections();
  } else {
    m_view->enablePolarizationCorrections();
  }
}

void ExperimentPresenter::updateFloodCorrectionEnabledState() {
  if (floodCorrectionRequiresInputs(
          m_model.floodCorrections().correctionType()))
    m_view->enableFloodCorrectionInputs();
  else
    m_view->disableFloodCorrectionInputs();
}

boost::optional<RangeInLambda>
ExperimentPresenter::transmissionRunRangeFromView() {
  auto const range = RangeInLambda(m_view->getTransmissionStartOverlap(),
                                   m_view->getTransmissionEndOverlap());
  auto const bothOrNoneMustBeSet = false;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showTransmissionRangeValid();
  else
    m_view->showTransmissionRangeInvalid();

  if (range.unset() || !range.isValid(bothOrNoneMustBeSet))
    return boost::none;
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
  if (maybeParamsList.is_initialized() && maybeParamsList->size() % 2 != 0) {
    m_view->showTransmissionStitchParamsValid();
    return stitchParams;
  }

  m_view->showTransmissionStitchParamsInvalid();
  return std::string();
}

TransmissionStitchOptions
ExperimentPresenter::transmissionStitchOptionsFromView() {
  auto transmissionRunRange = transmissionRunRangeFromView();
  auto stitchParams = transmissionStitchParamsFromView();
  auto scaleRHS = m_view->getTransmissionScaleRHSWorkspace();
  return TransmissionStitchOptions(transmissionRunRange, stitchParams,
                                   scaleRHS);
}

std::map<std::string, std::string>
ExperimentPresenter::stitchParametersFromView() {
  auto maybeStitchParameters = parseOptions(m_view->getStitchOptions());
  if (maybeStitchParameters.is_initialized()) {
    m_view->showStitchParametersValid();
    return maybeStitchParameters.get();
  }

  m_view->showStitchParametersInvalid();
  return std::map<std::string, std::string>();
}

ExperimentValidationResult ExperimentPresenter::validateExperimentFromView() {
  auto validate = PerThetaDefaultsTableValidator();
  auto perThetaValidationResult =
      validate(m_view->getPerAngleOptions(), m_thetaTolerance);
  if (perThetaValidationResult.isValid()) {
    auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
    auto const reductionType =
        reductionTypeFromString(m_view->getReductionType());
    auto const summationType =
        summationTypeFromString(m_view->getSummationType());
    auto const includePartialBins = m_view->getIncludePartialBins();
    auto const debugOption = m_view->getDebugOption();
    auto transmissionStitchOptions = transmissionStitchOptionsFromView();
    auto polarizationCorrections = polarizationCorrectionsFromView();
    auto floodCorrections = floodCorrectionsFromView();
    auto stitchParameters = stitchParametersFromView();
    return ExperimentValidationResult(
        Experiment(analysisMode, reductionType, summationType,
                   includePartialBins, debugOption, polarizationCorrections,
                   floodCorrections, transmissionStitchOptions,
                   stitchParameters, perThetaValidationResult.assertValid()));
  } else {
    return ExperimentValidationResult(
        ExperimentValidationErrors(perThetaValidationResult.assertError()));
  }
}

ExperimentValidationResult ExperimentPresenter::updateModelFromView() {
  auto validationResult = validateExperimentFromView();
  if (validationResult.isValid()) {
    m_model = validationResult.assertValid();
    updateWidgetEnabledState();
  }
  return validationResult;
}

void ExperimentPresenter::showPerThetaTableErrors(
    PerThetaDefaultsTableValidationError const &errors) {
  m_view->showAllPerAngleOptionsAsValid();
  for (auto const &validationError : errors.errors()) {
    for (auto const &column : validationError.invalidColumns())
      m_view->showPerAngleOptionsAsInvalid(validationError.row(), column);
  }
}

void ExperimentPresenter::showValidationResult(
    ExperimentValidationResult const &result) {
  if (result.isValid()) {
    m_view->showAllPerAngleOptionsAsValid();
  } else {
    auto errors = result.assertError();
    showPerThetaTableErrors(errors.perThetaValidationErrors());
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
  m_view->setPerAngleOptions(m_model.perThetaDefaultsArray());
  if (m_model.transmissionStitchOptions().overlapRange()) {
    m_view->setTransmissionStartOverlap(
        m_model.transmissionStitchOptions().overlapRange()->min());
    m_view->setTransmissionEndOverlap(
        m_model.transmissionStitchOptions().overlapRange()->max());
  } else {
    m_view->setTransmissionStartOverlap(0.0);
    m_view->setTransmissionEndOverlap(0.0);
  }
  m_view->setTransmissionStitchParams(
      m_model.transmissionStitchOptions().rebinParameters());
  m_view->setTransmissionScaleRHSWorkspace(
      m_model.transmissionStitchOptions().scaleRHS());
  m_view->setPolarizationCorrectionOption(
      m_model.polarizationCorrections().correctionType() !=
      PolarizationCorrectionType::None);
  m_view->setFloodCorrectionType(
      floodCorrectionTypeToString(m_model.floodCorrections().correctionType()));
  if (m_model.floodCorrections().workspace())
    m_view->setFloodWorkspace(m_model.floodCorrections().workspace().get());
  else
    m_view->setFloodWorkspace("");
  m_view->setStitchOptions(m_model.stitchParametersString());

  // We don't allow invalid config so reset all state to valid
  m_view->showAllPerAngleOptionsAsValid();
  m_view->showTransmissionRangeValid();
  m_view->showStitchParametersValid();

  updateWidgetEnabledState();

  // Reconnect settings change notifications
  m_view->connectExperimentSettingsWidgets();
}
} // namespace CustomInterfaces
} // namespace MantidQt
