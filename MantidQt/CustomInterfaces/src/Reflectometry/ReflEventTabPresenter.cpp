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


/** Returns global options for time-slicing for 'ReflectometryReductionOneAuto'
*
* @param group :: The group from which to get the options
* @return :: Global options for 'ReflectometryReductionOneAuto'
*/
std::string ReflEventTabPresenter::getTimeSlicingOptions(int group) const {

  return m_eventPresenters.at(group)->getTimeSlicingOptions();
}
}
}