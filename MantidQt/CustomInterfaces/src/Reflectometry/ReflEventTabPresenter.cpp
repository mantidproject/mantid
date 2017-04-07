#include "MantidQtCustomInterfaces/Reflectometry/ReflEventTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
*
* @param presenters :: The presenters of each group as a vector
*/
ReflEventTabPresenter::ReflEventTabPresenter(
    std::vector<IReflEventPresenter *> presenters)
    : m_eventPresenters(presenters) {}

/** Destructor
*
*/
ReflEventTabPresenter::~ReflEventTabPresenter() {}

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
}
}