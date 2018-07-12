#include "ExperimentPresenter.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"
#include "../../Reduction/ParseReflectometryStrings.h"
#include "PerThetaDefaultsTableValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

ExperimentPresenter::ExperimentPresenter(IExperimentView *view,
                                         double defaultsThetaTolerance)
    : m_view(view), m_model(), m_thetaTolerance(defaultsThetaTolerance) {
  m_view->subscribe(this);
  notifySettingsChanged();
}

void ExperimentPresenter::notifyNewPerAngleDefaultsRequested() {
  m_view->addPerThetaDefaultsRow();
  notifySettingsChanged();
}

void ExperimentPresenter::notifyRemovePerAngleDefaultsRequested(int index) {
  m_view->removePerThetaDefaultsRow(index);
  notifySettingsChanged();
}

PolarizationCorrections ExperimentPresenter::polarizationCorrectionsFromView() {
  auto const cRho = m_view->getCRho();
  auto const cAlpha = m_view->getCAlpha();
  auto const cAp = m_view->getCAp();
  auto const cPp = m_view->getCPp();
  return PolarizationCorrections(cRho, cAlpha, cAp, cPp);
}

RangeInLambda ExperimentPresenter::transmissionRunRangeFromView() {
  return RangeInLambda(m_view->getTransmissionStartOverlap(),
                       m_view->getTransmissionEndOverlap());
}

void ExperimentPresenter::notifySettingsChanged() {
  auto validationResult = updateModelFromView();
  showValidationResult(validationResult);
}

ExperimentValidationResult ExperimentPresenter::validateExperimentFromView() {
  auto validate = PerThetaDefaultsTableValidator();
  auto perThetaValidationResult =
      validate(m_view->getPerAngleOptions(), m_thetaTolerance);
  auto maybeStitchParameters = parseOptions(m_view->getStitchOptions());
  if (perThetaValidationResult.isValid() &&
      maybeStitchParameters.is_initialized()) {
    auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
    auto const reductionType =
        reductionTypeFromString(m_view->getReductionType());
    auto const summationType =
        summationTypeFromString(m_view->getSummationType());
    auto transmissionRunRange = transmissionRunRangeFromView();
    auto polarizationCorrections = polarizationCorrectionsFromView();
    return ExperimentValidationResult(Experiment(
        analysisMode, reductionType, summationType, polarizationCorrections,
        transmissionRunRange, maybeStitchParameters.get(),
        perThetaValidationResult.assertValid()));
  } else {
    return ExperimentValidationResult(
        ExperimentValidationErrors(perThetaValidationResult.assertError(),
                                   maybeStitchParameters.is_initialized()));
  }
}

ExperimentValidationResult ExperimentPresenter::updateModelFromView() {
  auto validationResult = validateExperimentFromView();
  m_model = validationResult.validElseNone();
  return validationResult;
}

void ExperimentPresenter::notifyPerAngleDefaultsChanged(int, int column) {
  auto validationResult = updateModelFromView();
  showValidationResult(validationResult);
  if (column == 0 && !validationResult.isValid() &&
      !validationResult.assertError()
           .perThetaValidationErrors()
           .hasUniqueThetas())
    m_view->showPerAngleThetasNonUnique(m_thetaTolerance);
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
    m_view->showStitchParametersValid();
  } else {
    auto errors = result.assertError();
    showPerThetaTableErrors(errors.perThetaValidationErrors());
    if (errors.stitchParametersAreValid())
      m_view->showStitchParametersValid();
    else
      m_view->showStitchParametersInvalid();
  }
}

void ExperimentPresenter::notifySummationTypeChanged() {
  if (m_model.get().summationType() == SummationType::SumInQ)
    m_view->enableReductionType();
  else
    m_view->disableReductionType();
}

Experiment const &ExperimentPresenter::experiment() const {
  return m_model.get();
}
}
}
