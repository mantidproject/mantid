#include "ReflEventPresenter.h"
#include "IReflEventTabPresenter.h"
#include "IReflEventView.h"

#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param view :: The view we are handling
 * @param group :: The group on the parent tab this belongs to
 */
ReflEventPresenter::ReflEventPresenter(IReflEventView *view, int group)
    : m_view(view), m_sliceType(SliceType::UniformEven), m_group(group) {
  m_view->enableSliceType(m_sliceType);
}

/** Destructor
 */
ReflEventPresenter::~ReflEventPresenter() {}

void ReflEventPresenter::acceptTabPresenter(
    IReflEventTabPresenter *tabPresenter) {
  m_tabPresenter = tabPresenter;
}

/** Returns the time-slicing values
 * @return :: The time-slicing values
 */
std::string ReflEventPresenter::getTimeSlicingValues() const {
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

std::string ReflEventPresenter::logFilterAndSliceValues(
    std::string const &slicingValues, std::string const &logFilter) const {
  if (!slicingValues.empty() && !logFilter.empty())
    return "Slicing=\"" + slicingValues + "\",LogFilter=" + logFilter;
  else
    return "";
}

/** Returns the time-slicing type
 * @return :: The time-slicing type
 */
std::string ReflEventPresenter::getTimeSlicingType() const {
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

void ReflEventPresenter::onReductionPaused() {
  m_view->enableSliceType(m_sliceType);
  m_view->enableSliceTypeSelection();
}

void ReflEventPresenter::onReductionResumed() {
  m_view->disableSliceType(m_sliceType);
  m_view->disableSliceTypeSelection();
}

void ReflEventPresenter::notifySliceTypeChanged(SliceType newSliceType) {
  m_view->disableSliceType(m_sliceType);
  m_view->enableSliceType(newSliceType);
  m_sliceType = newSliceType;
}

void ReflEventPresenter::notifySettingsChanged() {
  m_tabPresenter->settingsChanged(m_group);
}
} // namespace CustomInterfaces
} // namespace MantidQt
