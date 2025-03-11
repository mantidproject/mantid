// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FacilityInfo.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::FacilityInfo;
using Mantid::Kernel::InstrumentInfo;
using namespace boost::python;

void export_FacilityInfo() {

  register_ptr_to_python<FacilityInfo *>();

  class_<FacilityInfo>("FacilityInfo", no_init)
      .def("name", &FacilityInfo::name, arg("self"), return_value_policy<copy_const_reference>(),
           "Returns name of the facility as definined in the Facilities.xml "
           "file")
      .def("__str__", &FacilityInfo::name, arg("self"), return_value_policy<copy_const_reference>(),
           "Returns name of the facility as definined in the Facilities.xml "
           "file")
      .def("zeroPadding", &FacilityInfo::zeroPadding, arg("self"), "Returns default zero padding for this facility")
      .def("delimiter", &FacilityInfo::delimiter, arg("self"), return_value_policy<copy_const_reference>(),
           "Returns the delimiter between the instrument name and the run "
           "number.")
      .def("extensions", &FacilityInfo::extensions, arg("self"),
           "Returns the list of file extensions that are considered as "
           "instrument data files.")
      .def("preferredExtension", &FacilityInfo::preferredExtension, arg("self"),
           return_value_policy<copy_const_reference>(), "Returns the extension that is preferred for this facility")
      .def("timezone", &FacilityInfo::timezone, arg("self"), return_value_policy<copy_const_reference>(),
           "Returns the timezone appropriate for the facility. If there is not "
           "a timezone specified this returns an empty string.")
      .def("archiveSearch", &FacilityInfo::archiveSearch, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the archive search interface names")
      .def("instruments", (const std::vector<InstrumentInfo> &(FacilityInfo::*)() const) & FacilityInfo::instruments,
           arg("self"), return_value_policy<copy_const_reference>(),
           "Returns a list of instruments of this facility as defined in the "
           "Facilities.xml file")
      .def("instruments",
           // cppcheck-suppress cstyleCast
           (std::vector<InstrumentInfo>(FacilityInfo::*)(const std::string &) const) & FacilityInfo::instruments,
           (arg("self"), arg("technique")), "Returns a list of instruments of given technique")
      .def("instrument", &FacilityInfo::instrument, (arg("self"), arg("instrumentName")),
           return_value_policy<copy_const_reference>(), "Returns the instrument with the given name");
}
