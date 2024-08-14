// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "EventPresenter.h"
#include "Common/Parse.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "IEventPresenter.h"
#include "IEventView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/algorithm/string.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param view :: The view we are handling
 */
EventPresenter::EventPresenter(IEventView *view) : m_view(view), m_sliceType(SliceType::None) {
  m_view->subscribe(this);
}

void EventPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

Slicing const &EventPresenter::slicing() const { return m_slicing; }

void EventPresenter::notifyUniformSliceCountChanged(int) {
  if (m_sliceType == SliceType::UniformEven) {
    setUniformSlicingByNumberOfSlicesFromView();
    m_mainPresenter->notifySettingsChanged();
  }
}

void EventPresenter::notifyUniformSecondsChanged(double) {
  if (m_sliceType == SliceType::Uniform) {
    setUniformSlicingByTimeFromView();
    m_mainPresenter->notifySettingsChanged();
  }
}

void EventPresenter::notifyCustomSliceValuesChanged(std::string) {
  if (m_sliceType == SliceType::Custom) {
    setCustomSlicingFromView();
    m_mainPresenter->notifySettingsChanged();
  }
}

void EventPresenter::notifyLogSliceBreakpointsChanged(std::string) {
  if (m_sliceType == SliceType::LogValue) {
    setLogValueSlicingFromView();
    m_mainPresenter->notifySettingsChanged();
  }
}

void EventPresenter::notifyLogBlockNameChanged(std::string) {
  if (m_sliceType == SliceType::LogValue) {
    setLogValueSlicingFromView();
    m_mainPresenter->notifySettingsChanged();
  }
}

void EventPresenter::notifySliceTypeChanged(SliceType newSliceType) {
  m_view->disableSliceType(m_sliceType);
  m_view->enableSliceType(newSliceType);
  m_sliceType = newSliceType;
  setSlicingFromView();
  m_mainPresenter->notifySettingsChanged();
}

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void EventPresenter::updateWidgetEnabledState() const {
  if (isProcessing() || isAutoreducing()) {
    m_view->disableSliceType(m_sliceType);
    m_view->disableSliceTypeSelection();
  } else {
    m_view->enableSliceType(m_sliceType);
    m_view->enableSliceTypeSelection();
  }
}

void EventPresenter::notifyReductionPaused() { updateWidgetEnabledState(); }

void EventPresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void EventPresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void EventPresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

GNU_DIAG_OFF("maybe-uninitialized")

void EventPresenter::setUniformSlicingByTimeFromView() {
  m_slicing = UniformSlicingByTime(m_view->uniformSliceLength());
}

void EventPresenter::setUniformSlicingByNumberOfSlicesFromView() {
  m_slicing = UniformSlicingByNumberOfSlices(m_view->uniformSliceCount());
}

void EventPresenter::setCustomSlicingFromView() {
  auto maybeCustomBreakpoints = parseList(m_view->customBreakpoints(), parseNonNegativeDouble);
  if (maybeCustomBreakpoints.is_initialized()) {
    m_view->showCustomBreakpointsValid();
    m_slicing = CustomSlicingByList(maybeCustomBreakpoints.get());
  } else {
    m_view->showCustomBreakpointsInvalid();
    m_slicing = InvalidSlicing();
  }
}

void EventPresenter::setLogValueSlicingFromView() {
  auto maybeBreakpoints = parseList(m_view->logBreakpoints(), parseNonNegativeDouble);
  auto blockName = m_view->logBlockName();
  // Currently we don't support multiple log intervals, so for now if we have
  // more than one item in the list then show that as invalid. We intend to add
  // this though which is why this is still a free text field rather than a
  // spin box
  if (maybeBreakpoints.is_initialized() && maybeBreakpoints.get().size() <= 1) {
    m_view->showLogBreakpointsValid();
    m_slicing = SlicingByEventLog(maybeBreakpoints.get(), blockName);
  } else {
    m_view->showLogBreakpointsInvalid();
    m_slicing = InvalidSlicing();
  }
}

void EventPresenter::setSlicingFromView() {
  switch (m_sliceType) {
  case SliceType::UniformEven:
    setUniformSlicingByNumberOfSlicesFromView();
    break;
  case SliceType::Uniform:
    setUniformSlicingByTimeFromView();
    break;
  case SliceType::Custom:
    setCustomSlicingFromView();
    break;
  case SliceType::LogValue:
    setLogValueSlicingFromView();
    break;
  case SliceType::None:
    m_slicing = boost::blank();
    break;
  default:
    throw std::runtime_error("Unrecognized slice type.");
  }
}

GNU_DIAG_ON("maybe-uninitialized")

bool EventPresenter::isProcessing() const { return m_mainPresenter->isProcessing(); }

bool EventPresenter::isAutoreducing() const { return m_mainPresenter->isAutoreducing(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
