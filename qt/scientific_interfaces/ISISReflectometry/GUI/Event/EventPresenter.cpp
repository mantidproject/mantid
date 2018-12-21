// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EventPresenter.h"
#include "IEventPresenter.h"
#include "IEventView.h"
#include "Parse.h"
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param view :: The view we are handling
 */
EventPresenter::EventPresenter(IEventView *view)
    : m_view(view), m_sliceType(SliceType::None) {
  m_view->subscribe(this);
}

Slicing const &EventPresenter::slicing() const { return m_slicing; }

void EventPresenter::notifyUniformSliceCountChanged(int) {
  setUniformSlicingByNumberOfSlicesFromView();
}

void EventPresenter::notifyUniformSecondsChanged(double) {
  setUniformSlicingByTimeFromView();
}

void EventPresenter::notifyCustomSliceValuesChanged(std::string) {
  setCustomSlicingFromView();
}

void EventPresenter::notifyLogSliceBreakpointsChanged(std::string) {
  setLogValueSlicingFromView();
}

void EventPresenter::notifyLogBlockNameChanged(std::string) {
  setLogValueSlicingFromView();
}

void EventPresenter::reductionPaused() {
  m_view->enableSliceType(m_sliceType);
  m_view->enableSliceTypeSelection();
}

void EventPresenter::reductionResumed() {
  m_view->disableSliceType(m_sliceType);
  m_view->disableSliceTypeSelection();
}

void EventPresenter::autoreductionPaused() { reductionPaused(); }

void EventPresenter::autoreductionResumed() { reductionResumed(); }

void EventPresenter::setUniformSlicingByTimeFromView() {
  m_slicing = UniformSlicingByTime(m_view->uniformSliceLength());
}

void EventPresenter::setUniformSlicingByNumberOfSlicesFromView() {
  m_slicing = UniformSlicingByNumberOfSlices(m_view->uniformSliceCount());
}

void EventPresenter::setCustomSlicingFromView() {
  auto maybeCustomBreakpoints =
      parseList(m_view->customBreakpoints(), parseNonNegativeDouble);
  if (maybeCustomBreakpoints.is_initialized()) {
    m_view->showCustomBreakpointsValid();
    m_slicing = CustomSlicingByList(maybeCustomBreakpoints.get());
  } else {
    m_view->showCustomBreakpointsInvalid();
    m_slicing = InvalidSlicing();
  }
}

void EventPresenter::setLogValueSlicingFromView() {
  auto maybeBreakpoints =
      parseList(m_view->logBreakpoints(), parseNonNegativeDouble);
  auto blockName = m_view->logBlockName();
  if (maybeBreakpoints.is_initialized()) {
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

void EventPresenter::notifySliceTypeChanged(SliceType newSliceType) {
  std::cout << m_slicing << std::endl;
  m_view->disableSliceType(m_sliceType);
  m_view->enableSliceType(newSliceType);
  m_sliceType = newSliceType;
  setSlicingFromView();
}
} // namespace CustomInterfaces
} // namespace MantidQt
