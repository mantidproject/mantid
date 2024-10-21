// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>

GNU_DIAG_OFF("strict-aliasing")

using Mantid::API::IMaskWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IMaskWorkspace)

namespace {

bool isMaskedFromList(const IMaskWorkspace &self, const boost::python::list &ids) {
  std::set<Mantid::detid_t> idSet;
  auto length = len(ids);
  for (auto i = 0; i < length; ++i) {
    idSet.insert(extract<Mantid::detid_t>(ids[i])());
  }
  return self.isMasked(idSet);
}
} // namespace

void export_IMaskWorkspace() {
  class_<IMaskWorkspace, boost::noncopyable>("IMaskWorkspace", no_init)
      .def("getNumberMasked", &IMaskWorkspace::getNumberMasked, arg("self"),
           "Returns the number of masked pixels in the workspace")
      .def("isMasked", (bool(IMaskWorkspace::*)(const Mantid::detid_t) const) & IMaskWorkspace::isMasked,
           (arg("self"), arg("detector_id")), "Returns whether the given detector ID is masked")
      .def("isMasked", isMaskedFromList, (arg("self"), arg("detector_id_list")),
           "Returns whether all of the given detector ID list are masked");

  // register pointers
  RegisterWorkspacePtrToPython<IMaskWorkspace>();
}
