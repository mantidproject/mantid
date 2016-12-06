#include "MantidQtCustomInterfaces/Reflectometry/ReflEventPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventView.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: The view we are handling
*/
ReflEventPresenter::ReflEventPresenter(IReflEventView *view) : m_view(view) {}

/** Destructor
*/
ReflEventPresenter::~ReflEventPresenter() {}
}
}