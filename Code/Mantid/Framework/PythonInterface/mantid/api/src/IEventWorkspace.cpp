#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::IEventWorkspace;
using Mantid::API::IEventWorkspace_sptr;
using Mantid::API::IEventList;
using Mantid::API::WorkspaceProperty;
using Mantid::API::IWorkspaceProperty;
using Mantid::Kernel::PropertyWithValue;
using namespace boost::python;

void export_IEventWorkspace()
{
  class_<IEventWorkspace, bases<Mantid::API::MatrixWorkspace>, boost::noncopyable>("IEventWorkspace", no_init)
    .def("getNumberEvents", &IEventWorkspace::getNumberEvents, 
         "Returns the number of events in the workspace")
    .def("getTofMin", &IEventWorkspace::getTofMin, 
         "Returns the minimum TOF value (in microseconds) held by the workspace")
    .def("getTofMax", &IEventWorkspace::getTofMax, 
         "Returns the maximum TOF value (in microseconds) held by the workspace")
    .def("getEventList", (IEventList*(IEventWorkspace::*)(const int) ) &IEventWorkspace::getEventListPtr,
        return_internal_reference<>(), "Return the event list managing the events at the given workspace index")
    .def("clearMRU", &IEventWorkspace::clearMRU, "Clear the most-recently-used lists")
    ;

  DECLARE_SINGLEVALUETYPEHANDLER(IEventWorkspace, Mantid::Kernel::DataItem_sptr);
}

void export_IEventWorkspaceProperty()
{
  EXPORT_PROP_W_VALUE(IEventWorkspace_sptr, IEventWorkspace);
  register_ptr_to_python<WorkspaceProperty<IEventWorkspace>*>();
  class_<WorkspaceProperty<IEventWorkspace>, bases<PropertyWithValue<IEventWorkspace_sptr>,IWorkspaceProperty>,
         boost::noncopyable>("WorkspaceProperty_IEventWorkspace", no_init)
      ;
}
