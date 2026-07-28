// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/ICompAssembly.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ICompAssembly)

namespace {
// Deprecation wrappers. Legacy component-tree navigation is being retired in
// favour of the index-based ComponentInfo access layer (children,
// componentsInSubtree, detectorsInSubtree). See the 'Instrument Access via
// SpectrumInfo, DetectorInfo, ComponentInfo' concept page.
int nelementsDeprecated(const ICompAssembly &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'ICompAssembly.nelements'/'len()' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.componentsInSubtree' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.nelements();
}

std::shared_ptr<IComponent> getItemDeprecated(const ICompAssembly &self, const int index) {
  PyErr_Warn(PyExc_DeprecationWarning, "'ICompAssembly.__getitem__' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.children'/'componentsInSubtree' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self[index];
}
} // namespace

void export_ICompAssembly() {
  register_ptr_to_python<std::shared_ptr<ICompAssembly>>();

  class_<ICompAssembly, boost::python::bases<IComponent>, boost::noncopyable>("ICompAssembly", no_init)
      .def("nelements", &nelementsDeprecated, arg("self"),
           "Returns the number of elements in the assembly "
           "(deprecated, use ComponentInfo.componentsInSubtree)")
      .def("__len__", &nelementsDeprecated, arg("self"),
           "Returns the number of elements in the assembly "
           "(deprecated, use ComponentInfo.componentsInSubtree)")
      .def("__getitem__", &getItemDeprecated, (arg("self"), arg("index")),
           "Return the component at the given index "
           "(deprecated, use ComponentInfo.children/componentsInSubtree)");
}
