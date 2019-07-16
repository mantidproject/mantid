// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SymmetryElementFactoryImpl)

void export_SymmetryElementFactory() {
  class_<SymmetryElementFactoryImpl, boost::noncopyable>(
      "SymmetryElementFactoryImpl", no_init)
      .def("createSymElement", &SymmetryElementFactoryImpl::createSymElement,
           (arg("self"), arg("operation")),
           "Creates the symmetry element that corresponds to the supplied "
           "symmetry operation.")
      .def("Instance", &SymmetryElementFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the SymmetryElementFactory singleton")
      .staticmethod("Instance");
}
