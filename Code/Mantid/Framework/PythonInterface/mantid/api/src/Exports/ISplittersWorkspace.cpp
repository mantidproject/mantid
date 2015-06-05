#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_IPeaksWorkspace() {
  register_ptr_to_python<boost::shared_ptr<ISplittersWorkspace>>();

  // ISplittersWorkspace class
  class_<ISplittersWorkspace, bases<ITableWorkspace>, boost::noncopyable>(
      "ISplittersWorkspace", no_init)
      .def("getNumberSplitters", &ISplittersWorkspace::getNumberSplitters,
           "Returns the number of splitters within the workspace")
      .def("addSplitter", &IPeaksWorkspace::addSplitter,
           "Add a splitter to the workspace")
      .def("removeSplitter", &IPeaksWorkspace::removeSplitter,
           "Remove splitter from the workspace")
      .def("getSplitter", &IPeaksWorkspace::getSplitter,
           return_internal_reference<>(),
           "Returns a splitter at the given index");

  REGISTER_SINGLEVALUE_HANDLER(IPeaksWorkspace_sptr);
}
