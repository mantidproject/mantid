#include "InstrumentPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

namespace {
boost::optional<RangeInLambda> rangeOrNone(RangeInLambda &range,
                                           bool bothOrNoneMustBeSet) {
  if (range.unset() || !range.isValid(bothOrNoneMustBeSet))
    return boost::none;
  else
    return range;
}
} // namespace

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

boost::optional<RangeInLambda> InstrumentPresenter::wavelengthRangeFromView() {
  auto range = RangeInLambda(m_view->getLambdaMin(), m_view->getLambdaMax());
  auto const bothOrNoneMustBeSet = false;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showLambdaRangeValid();
  else
    m_view->showLambdaRangeInvalid();

  return rangeOrNone(range, bothOrNoneMustBeSet);
}

boost::optional<RangeInLambda>
InstrumentPresenter::monitorBackgroundRangeFromView() {
  auto range = RangeInLambda(m_view->getMonitorBackgroundMin(),
                             m_view->getMonitorBackgroundMax());
  auto const bothOrNoneMustBeSet = true;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showMonitorBackgroundRangeValid();
  else
    m_view->showMonitorBackgroundRangeInvalid();

  return rangeOrNone(range, bothOrNoneMustBeSet);
}

boost::optional<RangeInLambda>
InstrumentPresenter::monitorIntegralRangeFromView() {
  auto range = RangeInLambda(m_view->getMonitorIntegralMin(),
                             m_view->getMonitorIntegralMax());
  auto const bothOrNoneMustBeSet = false;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showMonitorIntegralRangeValid();
  else
    m_view->showMonitorIntegralRangeInvalid();

  return rangeOrNone(range, bothOrNoneMustBeSet);
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
  if (correctPositions)
    m_view->enableDetectorCorrectionType();
  else
    m_view->disableDetectorCorrectionType();
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
