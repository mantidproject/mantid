// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/ICompAssembly.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ICompAssembly)

void export_ICompAssembly() {
  register_ptr_to_python<boost::shared_ptr<ICompAssembly>>();

  class_<ICompAssembly, boost::python::bases<IComponent>, boost::noncopyable>(
      "ICompAssembly", no_init)
      .def("nelements", &ICompAssembly::nelements, arg("self"),
           "Returns the number of elements in the assembly")
      .def("__len__", &ICompAssembly::nelements, arg("self"),
           "Returns the number of elements in the assembly")
      .def("__getitem__",
           &ICompAssembly::operator[],(arg("self"), arg("index")),
           "Return the component at the given index");
}
