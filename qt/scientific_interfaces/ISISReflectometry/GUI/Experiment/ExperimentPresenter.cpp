#include "ExperimentPresenter.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

ExperimentPresenter::ExperimentPresenter(IExperimentView *view)
    : m_view(view), m_model() {
  m_view->subscribe(this);
}

void ExperimentPresenter::notifySettingsChanged() {
  auto const analysisMode = analysisModeFromString(m_view->getAnalysisMode());
  auto const reductionType =
      reductionTypeFromString(m_view->getReductionType());
  auto const summationType =
      summationTypeFromString(m_view->getSummationType());
  auto const cRho = m_view->getCRho();
  auto const cAlpha = m_view->getCAlpha();
  auto const cAp = m_view->getCAp();
  auto const cPp = m_view->getCPp();
  auto polarizationCorrections =
      PolarizationCorrections(cRho, cAlpha, cAp, cPp);
  auto transmissionRunRange =
      RangeInLambda(m_view->getTransmissionStartOverlap(),
                    m_view->getTransmissionEndOverlap());
  auto stitchParameters = m_view->getStitchOptions();

  auto rowTemplatesContent = m_view->getPerAngleOptions();

  std::vector<PerThetaDefaults> rowTemplates;
  auto row = 0;
  for (auto const& rowTemplateContent : rowTemplatesContent) {
    auto rowValidationResult = validatePerThetaDefaults(rowTemplateContent);
    if (rowValidationResult.isValid()) {
      rowTemplates.emplace_back(rowValidationResult.validRowElseNone().get());
      m_view->showPerAngleOptionsAsValid(row);
    } else {
      for (auto const& errorColumn : rowValidationResult.invalidColumns())
        m_view->showPerAngleOptionsAsInvalid(row, errorColumn);
    }
    row++;
  }

  m_model = Experiment(analysisMode, reductionType, summationType,
                       polarizationCorrections, transmissionRunRange,
                       stitchParameters, rowTemplates);
}

void ExperimentPresenter::notifySummationTypeChanged() {
  if (m_model.get().summationType() == SummationType::SumInQ)
    m_view->enableReductionType();
  else
    m_view->disableReductionType();
}

Experiment const &ExperimentPresenter::experiment() const { return m_model.get(); }
}
}
