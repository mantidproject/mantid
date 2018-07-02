#include "ExperimentPresenter.h"

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

  m_model = Experiment(analysisMode, reductionType, summationType,
                       polarizationCorrections, transmissionRunRange,
                       stitchParameters, {});
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
