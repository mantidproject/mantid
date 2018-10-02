#include "ExperimentPresenter.h"
#include "../../Reduction/ParseReflectometryStrings.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"
#include "PerThetaDefaultsTableValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

ExperimentPresenter::ExperimentPresenter(IExperimentView *view,
                                         Experiment experiment,
                                         double defaultsThetaTolerance)
    : m_view(view), m_model(std::move(experiment)),
      m_thetaTolerance(defaultsThetaTolerance) {
  m_view->subscribe(this);
  notifySettingsChanged();
}

Experiment const &ExperimentPresenter::experiment() const { return m_model; }

void ExperimentPresenter::notifySettingsChanged() {
  auto validationResult = updateModelFromView();
  showValidationResult(validationResult);
}

void ExperimentPresenter::notifySummationTypeChanged() {
  notifySettingsChanged();
  if (m_model.summationType() == SummationType::SumInQ)
    m_view->enableReductionType();
  else
    m_view->disableReductionType();
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
}

void ExperimentPresenter::onReductionPaused() { m_view->enableAll(); }

void ExperimentPresenter::onReductionResumed() { m_view->disableAll(); }

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

boost::optional<RangeInLambda>
ExperimentPresenter::transmissionRunRangeFromView() {
  auto const range = RangeInLambda(m_view->getTransmissionStartOverlap(),
                                   m_view->getTransmissionEndOverlap());
  if (range.isValid(false)) {
    m_view->showTransmissionRangeValid();
    return range;
  }

  m_view->showTransmissionRangeInvalid();
  return boost::none;
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
  auto stitchParameters = stitchParametersFromView();
  if (perThetaValidationResult.isValid()) {
    auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
    auto const reductionType =
        reductionTypeFromString(m_view->getReductionType());
    auto const summationType =
        summationTypeFromString(m_view->getSummationType());
    auto transmissionRunRange = transmissionRunRangeFromView();
    auto polarizationCorrections = polarizationCorrectionsFromView();
    return ExperimentValidationResult(
        Experiment(analysisMode, reductionType, summationType,
                   polarizationCorrections, transmissionRunRange,
                   stitchParameters, perThetaValidationResult.assertValid()));
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
