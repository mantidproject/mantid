#include "ExperimentPresenter.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"
#include "PerThetaDefaultsValidationResult.h"
// TODO
// - Adjust validation to handle the default angle case.

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

PerThetaDefaultsValidationResult validatePerThetaDefaultsFromView(
    std::vector<std::array<std::string, 8>> const &perThetaDefaultsContent,
    double thetaTolerance) {
  auto defaults = std::vector<PerThetaDefaults>();
  auto validationErrors = std::vector<InvalidDefaultsError>();

  auto row = 0;
  for (auto const &rowTemplateContent : perThetaDefaultsContent) {
    auto rowValidationResult = validatePerThetaDefaults(rowTemplateContent);
    if (rowValidationResult.isValid())
      defaults.emplace_back(rowValidationResult.validRowElseNone().get());
    else
      validationErrors.emplace_back(row, rowValidationResult.invalidColumns());
    row++;
  }

  auto defaultValidationResult =
      Experiment::validateThetaValues(defaults, thetaTolerance);
  if (defaultValidationResult == ThetaValuesValidationResult::Ok) {
    return PerThetaDefaultsValidationResult(std::move(defaults),
                                            std::move(validationErrors), true);
  } else if (defaultValidationResult ==
             ThetaValuesValidationResult::MultipleWildcards) {
    for (auto row = 0u; row < perThetaDefaultsContent.size(); ++row)
      validationErrors.emplace_back(row, std::vector<int>({0}));
    return PerThetaDefaultsValidationResult(std::move(defaults),
                                            std::move(validationErrors), true);
  } else /* if (defaultValidationResult ==
             ThetaValuesValidationResult::NonUniqueTheta) */ {
    for (auto row = 0u; row < perThetaDefaultsContent.size(); ++row)
      validationErrors.emplace_back(row, std::vector<int>({0}));
    return PerThetaDefaultsValidationResult(std::move(defaults),
                                            std::move(validationErrors), false);
  }
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

ExperimentValidationResult ExperimentPresenter::updateModelFromView() {
  auto perThetaValidationResult = validatePerThetaDefaultsFromView(
      m_view->getPerAngleOptions(), m_thetaTolerance);
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

    m_model = Experiment(analysisMode, reductionType, summationType,
                         polarizationCorrections, transmissionRunRange,
                         maybeStitchParameters.get(),
                         perThetaValidationResult.defaults());
  } else {
    m_model = boost::none;
  }
  return ExperimentValidationResult(perThetaValidationResult,
                                    maybeStitchParameters.is_initialized());
}

void ExperimentPresenter::notifyPerAngleDefaultsChanged(int, int column) {
  auto validationResult = updateModelFromView();
  showPerThetaDefaultsValidationResult(validationResult.perThetaDefaults());
  if (column == 0 && !validationResult.perThetaDefaults().hasUniqueThetas())
    m_view->showPerAngleThetasNonUnique(m_thetaTolerance);
}

void ExperimentPresenter::showPerThetaDefaultsValidationResult(
    PerThetaDefaultsValidationResult const &result) {
  if (result.isValid()) {
    m_view->showAllPerAngleOptionsAsValid();
  } else {
    m_view->showAllPerAngleOptionsAsValid();
    for (auto const &validationError : result.errors()) {
      for (auto const &column : validationError.invalidColumns())
        m_view->showPerAngleOptionsAsInvalid(validationError.row(), column);
    }
  }
}

void ExperimentPresenter::showValidationResult(
    ExperimentValidationResult const &result) {
  showPerThetaDefaultsValidationResult(result.perThetaDefaults());
  if (result.stitchParametersAreValid())
    m_view->showStitchParametersValid();
  else
    m_view->showStitchParametersInvalid();
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
