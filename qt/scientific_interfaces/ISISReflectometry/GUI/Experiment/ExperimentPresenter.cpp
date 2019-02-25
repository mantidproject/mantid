// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentPresenter.h"
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

void ExperimentPresenter::notifySummationTypeChanged() {
  notifySettingsChanged();
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
void ExperimentPresenter::updateWidgetEnabledState() const {
  if (isProcessing() || isAutoreducing())
    m_view->disableAll();
  else
    m_view->enableAll();
}

void ExperimentPresenter::reductionPaused() { updateWidgetEnabledState(); }

void ExperimentPresenter::reductionResumed() { updateWidgetEnabledState(); }

void ExperimentPresenter::autoreductionPaused() { updateWidgetEnabledState(); }

void ExperimentPresenter::autoreductionResumed() { updateWidgetEnabledState(); }

void ExperimentPresenter::instrumentChanged(
    std::string const &instrumentName,
    Mantid::Geometry::Instrument_const_sptr instrument) {
  UNUSED_ARG(instrumentName);
  // TODO: set defaults for the given instrument
}

PolarizationCorrections ExperimentPresenter::polarizationCorrectionsFromView() {
  auto const correctionType = polarizationCorrectionTypeFromString(
      m_view->getPolarizationCorrectionType());

  if (polarizationCorrectionRequiresInputs(correctionType)) {
    m_view->enablePolarizationCorrectionInputs();
    return PolarizationCorrections(correctionType, m_view->getCRho(),
                                   m_view->getCAlpha(), m_view->getCAp(),
                                   m_view->getCPp());
  }

  m_view->disablePolarizationCorrectionInputs();
  return PolarizationCorrections(correctionType);
}

FloodCorrections ExperimentPresenter::floodCorrectionsFromView() {
  auto const correctionType =
      floodCorrectionTypeFromString(m_view->getFloodCorrectionType());

  if (floodCorrectionRequiresInputs(correctionType)) {
    m_view->enableFloodCorrectionInputs();
    return FloodCorrections(correctionType, m_view->getFloodWorkspace());
  }

  m_view->disableFloodCorrectionInputs();
  return FloodCorrections(correctionType);
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
  if (validationResult.isValid())
    m_model = validationResult.assertValid();
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
} // namespace CustomInterfaces
} // namespace MantidQt
