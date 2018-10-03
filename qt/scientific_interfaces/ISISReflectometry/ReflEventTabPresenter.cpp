// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflEventTabPresenter.h"
#include "IReflMainWindowPresenter.h"
#include "ReflEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 *
 * @param presenters :: The presenters of each group as a vector
 */
ReflEventTabPresenter::ReflEventTabPresenter(
    std::vector<IReflEventPresenter *> presenters)
    : m_eventPresenters(presenters) {
  passSelfToChildren(presenters);
}

void ReflEventTabPresenter::passSelfToChildren(
    std::vector<IReflEventPresenter *> const &children) {
  for (auto *presenter : children)
    presenter->acceptTabPresenter(this);
}

/** Destructor
 *
 */
ReflEventTabPresenter::~ReflEventTabPresenter() {}

void ReflEventTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

/** Returns global time-slicing values for 'ReflectometryReductionOneAuto'
 *
 * @param group :: The group from which to get the values
 * @return :: Time-slicing values for 'ReflectometryReductionOneAuto'
 */
std::string ReflEventTabPresenter::getTimeSlicingValues(int group) const {
  return m_eventPresenters.at(group)->getTimeSlicingValues();
}

/** Returns time-slicing type for 'ReflectometryReductionOneAuto'
 *
 * @param group :: The group from which to get the values
 * @return :: Time-slicing type for 'ReflectometryReductionOneAuto'
 */
std::string ReflEventTabPresenter::getTimeSlicingType(int group) const {
  return m_eventPresenters.at(group)->getTimeSlicingType();
}

void ReflEventTabPresenter::onReductionPaused(int group) {
  m_eventPresenters[group]->onReductionPaused();
}

void ReflEventTabPresenter::onReductionResumed(int group) {
  m_eventPresenters[group]->onReductionResumed();
}

void ReflEventTabPresenter::settingsChanged(int group) {
  m_mainPresenter->settingsChanged(group);
}
} // namespace CustomInterfaces
} // namespace MantidQt
