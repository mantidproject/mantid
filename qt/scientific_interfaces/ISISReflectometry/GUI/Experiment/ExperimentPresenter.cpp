// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentPresenter.h"
#include "ExperimentOptionDefaults.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "PerThetaDefaultsTableValidator.h"
#include "Reduction/ParseReflectometryStrings.h"
#include "Reduction/ValidatePerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

ExperimentPresenter::ExperimentPresenter(IExperimentView *view,
                                         Experiment experiment,
                                         double defaultsThetaTolerance)
    : m_view(view), m_model(std::move(experiment)),
      m_thetaTolerance(defaultsThetaTolerance) {
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
  m_model = experimentDefaults(m_mainPresenter->instrument());
  updateViewFromModel();
  m_mainPresenter->notifySettingsChanged();
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

void ExperimentPresenter::instrumentChanged(
    std::string const &instrumentName,
    Mantid::Geometry::Instrument_const_sptr instrument) {
  UNUSED_ARG(instrumentName);
  m_model = experimentDefaults(instrument);
  updateViewFromModel();
}

PolarizationCorrections ExperimentPresenter::polarizationCorrectionsFromView() {
  auto const correctionType = polarizationCorrectionTypeFromString(
      m_view->getPolarizationCorrectionType());

  if (polarizationCorrectionRequiresInputs(correctionType)) {
    return PolarizationCorrections(correctionType, m_view->getCRho(),
                                   m_view->getCAlpha(), m_view->getCAp(),
                                   m_view->getCPp());
  }

  return PolarizationCorrections(correctionType);
}

void ExperimentPresenter::updatePolarizationCorrectionEnabledState() {
  if (polarizationCorrectionRequiresInputs(
          m_model.polarizationCorrections().correctionType()))
    m_view->enablePolarizationCorrectionInputs();
  else
    m_view->disablePolarizationCorrectionInputs();
}

FloodCorrections ExperimentPresenter::floodCorrectionsFromView() {
  auto const correctionType =
      floodCorrectionTypeFromString(m_view->getFloodCorrectionType());

  if (floodCorrectionRequiresInputs(correctionType)) {
    return FloodCorrections(correctionType, m_view->getFloodWorkspace());
  }

  return FloodCorrections(correctionType);
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
    auto transmissionRunRange = transmissionRunRangeFromView();
    auto polarizationCorrections = polarizationCorrectionsFromView();
    auto floodCorrections = floodCorrectionsFromView();
    auto stitchParameters = stitchParametersFromView();
    return ExperimentValidationResult(
        Experiment(analysisMode, reductionType, summationType,
                   includePartialBins, debugOption, polarizationCorrections,
                   floodCorrections, transmissionRunRange, stitchParameters,
                   perThetaValidationResult.assertValid()));
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
  if (m_model.transmissionRunRange()) {
    m_view->setTransmissionStartOverlap(m_model.transmissionRunRange()->min());
    m_view->setTransmissionEndOverlap(m_model.transmissionRunRange()->max());
    if (m_model.transmissionRunRange()->isValid(false))
      m_view->showTransmissionRangeValid();
    else
      m_view->showTransmissionRangeInvalid();
  }
  m_view->setPolarizationCorrectionType(polarizationCorrectionTypeToString(
      m_model.polarizationCorrections().correctionType()));
  m_view->setFloodCorrectionType(
      floodCorrectionTypeToString(m_model.floodCorrections().correctionType()));
  if (m_model.floodCorrections().workspace())
    m_view->setFloodWorkspace(m_model.floodCorrections().workspace().get());
  m_view->setStitchOptions(m_model.stitchParametersString());
  // The model can't be created with invalid stitch parameters so always set
  // them
  // to be valid
  m_view->showStitchParametersValid();

  updateWidgetEnabledState();

  // Reconnect settings change notifications
  m_view->connectExperimentSettingsWidgets();
}
} // namespace CustomInterfaces
} // namespace MantidQt
