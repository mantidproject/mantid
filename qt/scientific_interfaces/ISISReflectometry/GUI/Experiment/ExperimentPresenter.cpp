#include "ExperimentPresenter.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"
#include "PerThetaDefaultsValidationResult.h"
// TODO
// - Separate table change notifications from other notifications.
// - Only show theta warning dialog for theta value changes.
// - Validate the stitch options input.
// - World domination.

namespace MantidQt {
namespace CustomInterfaces {

ExperimentPresenter::ExperimentPresenter(IExperimentView *view)
    : m_view(view), m_model() {
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

  if (Experiment::thetaValuesAreUnique(defaults, thetaTolerance)) {
    return PerThetaDefaultsValidationResult(std::move(defaults),
                                            std::move(validationErrors), true);
  } else {
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
  showPerThetaDefaultsValidationResult(validationResult);
}

PerThetaDefaultsValidationResult ExperimentPresenter::updateModelFromView() {
  auto perThetaValidationResult =
      validatePerThetaDefaultsFromView(m_view->getPerAngleOptions(), 0.01);
  if (perThetaValidationResult.isValid()) {
    auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
    auto const reductionType =
        reductionTypeFromString(m_view->getReductionType());
    auto const summationType =
        summationTypeFromString(m_view->getSummationType());
    auto transmissionRunRange = transmissionRunRangeFromView();
    auto stitchParameters = m_view->getStitchOptions();
    auto polarizationCorrections = polarizationCorrectionsFromView();

    m_model = Experiment(analysisMode, reductionType, summationType,
                         polarizationCorrections, transmissionRunRange,
                         stitchParameters, perThetaValidationResult.defaults());
  } else {
    m_model = boost::none;
  }
  return perThetaValidationResult;
}

void ExperimentPresenter::notifyPerAngleDefaultsChanged(int row, int column) {
  auto validationResult = updateModelFromView();
  showPerThetaDefaultsValidationResult(validationResult);
  if (column == 0 && !validationResult.hasUniqueThetas())
    m_view->showPerAngleThetasNonUnique(0.01);
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
