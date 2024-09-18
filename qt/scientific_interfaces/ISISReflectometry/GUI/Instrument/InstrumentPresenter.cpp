// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "InstrumentOptionDefaults.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <ostream>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");

boost::optional<RangeInLambda> rangeOrNone(const RangeInLambda &range, const bool bothOrNoneMustBeSet) {
  if (range.unset() || !range.isValid(bothOrNoneMustBeSet))
    return boost::none;
  else
    return range;
}
} // namespace

InstrumentPresenter::InstrumentPresenter(IInstrumentView *view, Instrument instrument, IFileHandler *fileHandler,
                                         IReflMessageHandler *messageHandler,
                                         std::unique_ptr<IInstrumentOptionDefaults> instrumentDefaults)
    : m_instrumentDefaults(std::move(instrumentDefaults)), m_view(view), m_model(std::move(instrument)),
      m_fileHandler(fileHandler), m_messageHandler(messageHandler) {
  m_view->subscribe(this);
}

void InstrumentPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

void InstrumentPresenter::notifySettingsChanged() {
  updateModelFromView();
  m_mainPresenter->notifySettingsChanged();
}

void InstrumentPresenter::notifyRestoreDefaultsRequested() {
  // Trigger a reload of the instrument to get up-to-date settings.
  m_mainPresenter->notifyUpdateInstrumentRequested();
  restoreDefaults();
}

void InstrumentPresenter::notifyBrowseToCalibrationFileRequested() {
  auto calibrationFilePath = m_messageHandler->askUserForLoadFileName("Data Files (*.dat)");
  if (!calibrationFilePath.empty()) {
    m_view->setCalibrationFilePath(calibrationFilePath);
  }
}

Instrument const &InstrumentPresenter::instrument() const { return m_model; }

bool InstrumentPresenter::isProcessing() const { return m_mainPresenter->isProcessing(); }

bool InstrumentPresenter::isAutoreducing() const { return m_mainPresenter->isAutoreducing(); }

/** Tells the view to update the enabled/disabled state of all widgets
 * depending on whether they are currently applicable or not
 */
void InstrumentPresenter::updateWidgetEnabledState() {
  if (isProcessing() || isAutoreducing())
    m_view->disableAll();
  else
    m_view->enableAll();

  if (m_model.detectorCorrections().correctPositions())
    m_view->enableDetectorCorrectionType();
  else
    m_view->disableDetectorCorrectionType();
}

/** Tells the view to update the valid/invalid state of all widgets
 * depending on whether their values in the model are valid or not
 */
void InstrumentPresenter::updateWidgetValidState() {
  // Check the ranges are valid. It's fine if they're not set at all,
  // so show them as valid if the range is not intialised

  if (!m_model.wavelengthRange() || m_model.wavelengthRange()->isValid(false))
    m_view->showLambdaRangeValid();
  else
    m_view->showLambdaRangeInvalid();

  if (!m_model.monitorBackgroundRange() || m_model.monitorBackgroundRange()->isValid(true))
    m_view->showMonitorBackgroundRangeValid();
  else
    m_view->showMonitorBackgroundRangeInvalid();

  if (!m_model.monitorIntegralRange() || m_model.monitorIntegralRange()->isValid(false))
    m_view->showMonitorIntegralRangeValid();
  else
    m_view->showMonitorIntegralRangeInvalid();

  updateCalibrationFileValidState(m_model.calibrationFilePath());
}

void InstrumentPresenter::updateCalibrationFileValidState(const std::string &calibrationFilePath) {
  if (!calibrationFilePath.empty() && !m_fileHandler->fileExists(calibrationFilePath)) {
    m_view->showCalibrationFilePathInvalid();
  } else {
    m_view->showCalibrationFilePathValid();
  }
}

void InstrumentPresenter::notifyReductionPaused() { updateWidgetEnabledState(); }

void InstrumentPresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void InstrumentPresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void InstrumentPresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

void InstrumentPresenter::notifyInstrumentChanged(std::string const &instrumentName) {
  UNUSED_ARG(instrumentName);
  restoreDefaults();
}

void InstrumentPresenter::restoreDefaults() {
  auto const instr = m_mainPresenter->instrument();
  try {
    m_model = m_instrumentDefaults->get(instr);
  } catch (std::invalid_argument &ex) {
    std::ostringstream msg;
    msg << "Error setting default Instrument Settings: " << ex.what() << ". Please check the " << instr->getName()
        << " parameters file.";
    g_log.error(msg.str());
    m_model = Instrument();
  }
  updateViewFromModel();
}

boost::optional<RangeInLambda> InstrumentPresenter::wavelengthRangeFromView() {
  auto range = RangeInLambda(m_view->getLambdaMin(), m_view->getLambdaMax());
  bool const bothOrNoneMustBeSet = false;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showLambdaRangeValid();
  else
    m_view->showLambdaRangeInvalid();

  return rangeOrNone(range, bothOrNoneMustBeSet);
}

boost::optional<RangeInLambda> InstrumentPresenter::monitorBackgroundRangeFromView() {
  auto range = RangeInLambda(m_view->getMonitorBackgroundMin(), m_view->getMonitorBackgroundMax());
  bool const bothOrNoneMustBeSet = true;

  if (range.isValid(bothOrNoneMustBeSet))
    m_view->showMonitorBackgroundRangeValid();
  else
    m_view->showMonitorBackgroundRangeInvalid();

  return rangeOrNone(range, bothOrNoneMustBeSet);
}

boost::optional<RangeInLambda> InstrumentPresenter::monitorIntegralRangeFromView() {
  auto range = RangeInLambda(m_view->getMonitorIntegralMin(), m_view->getMonitorIntegralMax());
  bool const bothOrNoneMustBeSet = false;

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
  return MonitorCorrections(monitorIndex, integrate, backgroundRange, integralRange);
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

std::string InstrumentPresenter::calibrationFilePathFromView() {
  auto const calibrationFilePath = m_view->getCalibrationFilePath();
  updateCalibrationFileValidState(calibrationFilePath);
  return calibrationFilePath;
}

void InstrumentPresenter::updateModelFromView() {
  auto const wavelengthRange = wavelengthRangeFromView();
  auto const monitorCorrections = monitorCorrectionsFromView();
  auto const detectorCorrections = detectorCorrectionsFromView();
  auto const calibrationFilePath = calibrationFilePathFromView();
  m_model = Instrument(wavelengthRange, monitorCorrections, detectorCorrections, calibrationFilePath);
}

void InstrumentPresenter::updateViewFromModel() {
  // Disconnect notifications about settings updates otherwise we'll end
  // up updating the model from the view after the first change
  m_view->disconnectInstrumentSettingsWidgets();

  if (m_model.wavelengthRange()) {
    m_view->setLambdaMin(m_model.wavelengthRange()->min());
    m_view->setLambdaMax(m_model.wavelengthRange()->max());
  }
  m_view->setMonitorIndex(static_cast<int>(m_model.monitorIndex()));
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
  m_view->setDetectorCorrectionType(detectorCorrectionTypeToString(m_model.detectorCorrectionType()));
  m_view->setCalibrationFilePath(m_model.calibrationFilePath());

  updateWidgetEnabledState();
  updateWidgetValidState();

  // Reconnect settings change notifications
  m_view->connectInstrumentSettingsWidgets();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
