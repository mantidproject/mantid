#include "MantidQtCustomInterfaces/Reflectometry/ReflEventPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventView.h"

#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: The view we are handling
*/
ReflEventPresenter::ReflEventPresenter(IReflEventView *view) : m_view(view) {}

/** Destructor
*/
ReflEventPresenter::~ReflEventPresenter() {}

/** Returns the time-slicing values
* @return :: The time-slicing values
*/
std::string ReflEventPresenter::getTimeSlicingValues() const {

  return m_view->getTimeSlicingValues();
}

/** Returns the time-slicing type
* @return :: The time-slicing type
*/
std::string ReflEventPresenter::getTimeSlicingType() const {

  return m_view->getTimeSlicingType();
}
}
}
