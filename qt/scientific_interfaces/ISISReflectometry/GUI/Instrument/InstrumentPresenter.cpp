// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "InstrumentOptionDefaults.h"
#include "MantidGeometry/Instrument_fwd.h"

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
}

void InstrumentPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
  notifySettingsChanged();
}

void InstrumentPresenter::notifySettingsChanged() {
  updateModelFromView();
  m_mainPresenter->notifySettingsChanged();
}

void InstrumentPresenter::notifyRestoreDefaultsRequested() {
  m_model = instrumentDefaults(m_mainPresenter->instrument());
  updateViewFromModel();
  m_mainPresenter->notifySettingsChanged();
}

Instrument const &InstrumentPresenter::instrument() const { return m_model; }

bool InstrumentPresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool InstrumentPresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void InstrumentPresenter::updateWidgetEnabledState() const {
  if (isProcessing() || isAutoreducing())
    m_view->disableAll();
  else
    m_view->enableAll();
}

void InstrumentPresenter::reductionPaused() { updateWidgetEnabledState(); }

void InstrumentPresenter::reductionResumed() { updateWidgetEnabledState(); }

void InstrumentPresenter::autoreductionPaused() { updateWidgetEnabledState(); }

void InstrumentPresenter::autoreductionResumed() { updateWidgetEnabledState(); }

void InstrumentPresenter::instrumentChanged(
    std::string const &instrumentName,
    Mantid::Geometry::Instrument_const_sptr instrument) {
  UNUSED_ARG(instrumentName);
  m_model = instrumentDefaults(instrument);
  updateViewFromModel();
}

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

void InstrumentPresenter::updateViewFromModel() {
  // Disconnect notifications about settings updates otherwise we'll end
  // up updating the model from the view after the first change
  m_view->disconnectInstrumentSettingsWidgets();

  if (m_model.wavelengthRange()) {
    m_view->setLambdaMin(m_model.wavelengthRange()->min());
    m_view->setLambdaMax(m_model.wavelengthRange()->max());
  }
  m_view->setIntegrateMonitors(m_model.integratedMonitors());
  if (m_model.monitorIntegralRange()) {
    m_view->setMonitorIntegralMin(m_model.monitorIntegralRange()->min());
    m_view->setMonitorIntegralMax(m_model.monitorIntegralRange()->max());
  }
  if (m_model.monitorBackgroundRange()) {
    m_view->setMonitorBackgroundMin(m_model.monitorBackgroundRange()->min());
    m_view->setMonitorBackgroundMax(m_model.monitorBackgroundRange()->max());
  }
  m_view->setCorrectDetectors(m_model.correctDetectors());
  m_view->setDetectorCorrectionType(
      detectorCorrectionTypeToString(m_model.detectorCorrectionType()));

  // Reconnect settings change notifications
  m_view->connectInstrumentSettingsWidgets();
}
} // namespace CustomInterfaces
} // namespace MantidQt
