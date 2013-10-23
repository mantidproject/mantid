#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>

using Mantid::API::ISplittersWorkspace;
using Mantid::API::ITableWorkspace;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

void export_IPeaksWorkspace()
{
  REGISTER_SHARED_PTR_TO_PYTHON(ISplittersWorkspace);

  // ISplittersWorkspace class
  class_< ISplittersWorkspace, bases<ITableWorkspace>, boost::noncopyable >("ISplittersWorkspace", no_init)
    .def("getNumberSplitters", &ISplittersWorkspace::getNumberSplitters, "Returns the number of splitters within the workspace")
    .def("addSplitter", &IPeaksWorkspace::addSplitter, "Add a splitter to the workspace")
    .def("removeSplitter", &IPeaksWorkspace::removeSplitter, "Remove splitter from the workspace")
    .def("getSplitter", &IPeaksWorkspace::getSplitter, return_internal_reference<>(), "Returns a splitter at the given index" )
      ;

  REGISTER_SINGLEVALUE_HANDLER(IPeaksWorkspace_sptr);

}
