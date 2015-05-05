#include "MantidAPI/IWorkspaceProperty.h"
#include <boost/python/class.hpp>

// clang-format off
void export_IWorkspaceProperty()
// clang-format on
{
  using namespace boost::python;
  using Mantid::API::IWorkspaceProperty;

  class_<IWorkspaceProperty, boost::noncopyable>("IWorkspaceProperty", no_init)
    .def("isOptional", &IWorkspaceProperty::isOptional, "Is the input workspace property optional")
    .def("isLocking", &IWorkspaceProperty::isLocking, "Will the workspace be locked when starting an algorithm")
  ;

}
