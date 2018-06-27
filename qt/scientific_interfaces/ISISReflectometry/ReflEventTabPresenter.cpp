#include "ReflEventTabPresenter.h"
#include "IReflEventTabPresenter.h"
#include "IReflEventTabView.h"

#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: The view we are handling
* @param group :: The group on the parent tab this belongs to
*/
ReflEventTabPresenter::ReflEventTabPresenter(IReflEventTabView *view)
    : m_view(view), m_sliceType(SliceType::UniformEven) {
  m_view->subscribe(this);
  m_view->enableSliceType(m_sliceType);
}

void ReflEventTabPresenter::acceptMainPresenter(
    IReflBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

/** Returns the time-slicing values
* @return :: The time-slicing values
*/
std::string ReflEventTabPresenter::getTimeSlicingValues() const {
  switch (m_sliceType) {
  case SliceType::UniformEven:
    return m_view->getUniformEvenTimeSlicingValues();
  case SliceType::Uniform:
    return m_view->getUniformTimeSlicingValues();
  case SliceType::Custom:
    return m_view->getCustomTimeSlicingValues();
  case SliceType::LogValue: {
    auto slicingValues = m_view->getLogValueTimeSlicingValues();
    auto logFilter = m_view->getLogValueTimeSlicingType();
    return logFilterAndSliceValues(slicingValues, logFilter);
  }
  default:
    throw std::runtime_error("Unrecognized slice type.");
  }
}

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
  default:
    throw std::runtime_error("Unrecognized slice type.");
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

void ReflEventTabPresenter::notifySliceTypeChanged(SliceType newSliceType) {
  m_view->disableSliceType(m_sliceType);
  m_view->enableSliceType(newSliceType);
  m_sliceType = newSliceType;
}

void ReflEventTabPresenter::notifySettingsChanged() {}
}
}
