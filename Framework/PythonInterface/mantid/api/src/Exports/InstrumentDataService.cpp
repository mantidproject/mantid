// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidPythonInterface/core/DataServiceExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(InstrumentDataServiceImpl)

namespace {

InstrumentDataServiceImpl &instance() { return InstrumentDataService::Instance(); }

Mantid::Geometry::Instrument_sptr retrieveOrKeyError(const InstrumentDataServiceImpl &self, const std::string &name) {
  // Can't use DataServiceExporter::retrieveOrKeyError here because it returns a weak pointer which we can't use with
  // Instrument. So we have to reimplement the same logic here, but returning a shared pointer instead.
  try {
    return self.retrieve(name);
  } catch (const Mantid::Kernel::Exception::NotFoundError &) {
    const std::string err = "'" + name + "' does not exist.";
    PyErr_SetString(PyExc_KeyError, err.c_str());
    throw boost::python::error_already_set();
  }
}

} // namespace

void export_InstrumentDataService() {
  using IDSExporter = DataServiceExporter<InstrumentDataServiceImpl, Mantid::Geometry::Instrument_sptr>;

  class_<InstrumentDataServiceImpl, boost::noncopyable>("InstrumentDataServiceImpl", no_init)
      .def("Instance", instance, return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance")
      .def("doesExist", &InstrumentDataServiceImpl::doesExist, (arg("self"), arg("name")),
           "Returns True if the object is found in the service.")
      .def("retrieve", retrieveOrKeyError, (arg("self"), arg("name")),
           "Retrieve the named object. Raises an exception if the name does not exist")
      .def("remove", &IDSExporter::removeItem, (arg("self"), arg("name")), "Remove a named object")
      .def("clear", &InstrumentDataServiceImpl::clear, arg("self"), "Removes all objects managed by the service.")
      .def("size", &InstrumentDataServiceImpl::size, arg("self"), "Returns the number of objects within the service")
      .def("getObjectNames", &IDSExporter::getObjectNamesAsList, (arg("self"), arg("contain") = ""),
           "Return the list of names currently known to the IDS");
}
