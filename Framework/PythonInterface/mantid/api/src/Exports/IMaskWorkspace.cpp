#include "MantidAPI/IMaskWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>

// clang-format off
GCC_DIAG_OFF(strict-aliasing)
// clang-format on

using Mantid::API::IMaskWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

namespace {

bool isMaskedFromList(const IMaskWorkspace &self,
                      const boost::python::list &ids) {
  std::set<Mantid::detid_t> idSet;
  auto length = len(ids);
  for (auto i = 0; i < length; ++i) {
    idSet.insert(extract<Mantid::detid_t>(ids[i])());
  }
  return self.isMasked(idSet);
}
}

void export_IMaskWorkspace() {
  class_<IMaskWorkspace, boost::noncopyable>("IMaskWorkspace", no_init)
      .def("getNumberMasked", &IMaskWorkspace::getNumberMasked,
           "Returns the number of masked pixels in the workspace")
      .def("isMasked", (bool (IMaskWorkspace::*)(const Mantid::detid_t) const) &
                           IMaskWorkspace::isMasked,
           "Returns whether the given detector ID is masked")
      .def("isMasked", isMaskedFromList,
           "Returns whether all of the given detector ID list are masked");

  // register pointers
  RegisterWorkspacePtrToPython<IMaskWorkspace>();
}
