#include "InstrumentPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

InstrumentPresenter::InstrumentPresenter(IInstrumentView *view,
                                         Instrument instrument)
    : m_view(view), m_model(std::move(instrument)) {
  m_view->subscribe(this);
  notifySettingsChanged();
}

void InstrumentPresenter::notifySettingsChanged() { updateModelFromView(); }

Instrument const &InstrumentPresenter::instrument() const { return m_model; }

void InstrumentPresenter::onReductionPaused() { m_view->enableAll(); }

void InstrumentPresenter::onReductionResumed() { m_view->disableAll(); }

RangeInLambda InstrumentPresenter::wavelengthRangeFromView() {
  auto const range =
      RangeInLambda(m_view->getLambdaMin(), m_view->getLambdaMax());
  if (range.isValid(false))
    m_view->showLambdaRangeValid();
  else
    m_view->showLambdaRangeInvalid();
  return range;
}

RangeInLambda InstrumentPresenter::monitorBackgroundRangeFromView() {
  auto const range = RangeInLambda(m_view->getMonitorBackgroundMin(),
                                   m_view->getMonitorBackgroundMax());
  if (range.isValid(true))
    m_view->showMonitorBackgroundRangeValid();
  else
    m_view->showMonitorBackgroundRangeInvalid();
  return range;
}

RangeInLambda InstrumentPresenter::monitorIntegralRangeFromView() {
  auto const range = RangeInLambda(m_view->getMonitorIntegralMin(),
                                   m_view->getMonitorIntegralMax());
  if (range.isValid(false))
    m_view->showMonitorIntegralRangeValid();
  else
    m_view->showMonitorIntegralRangeInvalid();
  return range;
}

MonitorCorrections InstrumentPresenter::monitorCorrectionsFromView() {
  auto const monitorIndex = m_view->getMonitorIndex();
  auto const integrate = m_view->getIntegrateMonitors();
  auto const backgroundRange = monitorBackgroundRangeFromView();
  auto const integralRange = monitorIntegralRangeFromView();
  return MonitorCorrections(monitorIndex, integrate, backgroundRange,
                            integralRange);
}

DetectorCorrectionType InstrumentPresenter::detectorCorrectionTypeFromView() {
  if (m_view->getDetectorCorrectionType() == "RotateAroundSample")
    return DetectorCorrectionType::RotateAroundSample;
  else
    return DetectorCorrectionType::VerticalShift;
}

DetectorCorrections InstrumentPresenter::detectorCorrectionsFromView() {
  auto const correctPositions = m_view->getCorrectDetectors();
  auto const correctionType = detectorCorrectionTypeFromView();
  return DetectorCorrections(correctPositions, correctionType);
}

void InstrumentPresenter::updateModelFromView() {
  auto const wavelengthRange = wavelengthRangeFromView();
  auto const monitorCorrections = monitorCorrectionsFromView();
  auto const detectorCorrections = detectorCorrectionsFromView();
  m_model =
      Instrument(wavelengthRange, monitorCorrections, detectorCorrections);
}
} // namespace CustomInterfaces
} // namespace MantidQt
