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
}
}