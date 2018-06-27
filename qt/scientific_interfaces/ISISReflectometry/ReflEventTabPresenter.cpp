#include "ReflEventTabPresenter.h"
#include "IReflEventTabPresenter.h"
#include "IReflEventTabView.h"
#include "Parse.h"
#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: The view we are handling
* @param group :: The group on the parent tab this belongs to
*/
ReflEventTabPresenter::ReflEventTabPresenter(IReflEventTabView *view)
    : m_view(view), m_sliceType(SliceType::None) {
  m_view->subscribe(this);
}

void ReflEventTabPresenter::acceptMainPresenter(
    IReflBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

Slicing const &ReflEventTabPresenter::slicing() const { return m_slicing; }

void ReflEventTabPresenter::notifyUniformSliceCountChanged(int) {
  setUniformSlicingByNumberOfSlicesFromView();
}

void ReflEventTabPresenter::notifyUniformSecondsChanged(double) {
  setUniformSlicingByTimeFromView();
}

void ReflEventTabPresenter::notifyCustomSliceValuesChanged(std::string) {
  setCustomSlicingFromView();
}

void ReflEventTabPresenter::notifyLogSliceBreakpointsChanged(std::string) {
  setLogValueSlicingFromView();
}

void ReflEventTabPresenter::notifyLogBlockNameChanged(std::string) {
  setLogValueSlicingFromView();
}

/** Returns the time-slicing values
* @return :: The time-slicing values
*/
std::string ReflEventTabPresenter::getTimeSlicingValues() const { return {}; }

std::string ReflEventTabPresenter::logFilterAndSliceValues(
    std::string const &slicingValues, std::string const &logFilter) const {
  if (!slicingValues.empty() && !logFilter.empty())
    return "Slicing=\"" + slicingValues + "\",LogFilter=" + logFilter;
  else
    return "";
}

/** Returns the time-slicing type
* @return :: The time-slicing type
*/
std::string ReflEventTabPresenter::getTimeSlicingType() const {
  switch (m_sliceType) {
  case SliceType::UniformEven:
    return "UniformEven";
  case SliceType::Uniform:
    return "Uniform";
  case SliceType::Custom:
    return "Custom";
  case SliceType::LogValue:
    return "LogValue";
  case SliceType::None:
    return "None";
  default:
    throw std::runtime_error("B Unrecognized slice type.");
  }
}

void ReflEventTabPresenter::onReductionPaused() {
  m_view->enableSliceType(m_sliceType);
  m_view->enableSliceTypeSelection();
}

void ReflEventTabPresenter::onReductionResumed() {
  m_view->disableSliceType(m_sliceType);
  m_view->disableSliceTypeSelection();
}

void ReflEventTabPresenter::setUniformSlicingByTimeFromView() {
  m_slicing = UniformSlicingByTime(m_view->uniformSliceLength());
}

void ReflEventTabPresenter::setUniformSlicingByNumberOfSlicesFromView() {
  m_slicing = UniformSlicingByNumberOfSlices(m_view->uniformSliceCount());
}

void ReflEventTabPresenter::setCustomSlicingFromView() {
  auto maybeCustomBreakpoints =
      parseList(m_view->customBreakpoints(), parseNonNegativeDouble);
  if (maybeCustomBreakpoints.is_initialized()) {
    m_view->showCustomBreakpointsValid();
    m_slicing = CustomSlicingByList(maybeCustomBreakpoints.get());
  } else {
    m_view->showCustomBreakpointsInvalid();
  }
}

void ReflEventTabPresenter::setLogValueSlicingFromView() {
  auto maybeBreakpoints =
      parseList(m_view->logBreakpoints(), parseNonNegativeDouble);
  auto blockName = m_view->logBlockName();
  if (maybeBreakpoints.is_initialized()) {
    m_view->showLogBreakpointsValid();
    m_slicing = SlicingByEventLog(maybeBreakpoints.get(), blockName);
  } else {
    m_view->showLogBreakpointsInvalid();
  }
}

void ReflEventTabPresenter::setSlicingFromView() {
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
    throw std::runtime_error("A Unrecognized slice type.");
  }
}

void ReflEventTabPresenter::notifySliceTypeChanged(SliceType newSliceType) {
  m_view->disableSliceType(m_sliceType);
  m_view->enableSliceType(newSliceType);
  m_sliceType = newSliceType;
  setSlicingFromView();
}
}
}
