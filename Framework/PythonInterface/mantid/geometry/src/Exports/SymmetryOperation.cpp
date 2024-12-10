// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidPythonInterface/core/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/core/StlExportDefinitions.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>

using Mantid::Geometry::SymmetryOperation;
using Mantid::PythonInterface::std_vector_exporter;

using namespace boost::python;

namespace //<unnamed>
{
using namespace Mantid::PythonInterface;

Mantid::Kernel::V3D applyToVector(const SymmetryOperation &self, const object &hkl) {
  return self.transformHKL(Converters::PyObjectToV3D(hkl)());
}

Mantid::Kernel::V3D applyToCoordinates(const SymmetryOperation &self, const object &coordinates) {
  return self.operator* <Mantid::Kernel::V3D>(Converters::PyObjectToV3D(coordinates)());
}
} // namespace

void export_SymmetryOperation() {
  register_ptr_to_python<std::shared_ptr<SymmetryOperation>>();

  class_<SymmetryOperation>("SymmetryOperation")
      .def("getOrder", &SymmetryOperation::order, arg("self"),
           "Returns the order of the symmetry operation, which indicates how "
           "often the operation needs to be applied to a point to arrive at "
           "identity.")
      .def("getIdentifier", &SymmetryOperation::identifier, arg("self"),
           "The identifier of the operation in x,y,z-notation.")
      .def("transformCoordinates", &applyToCoordinates, (arg("self"), arg("coordinates")),
           "Returns transformed coordinates. For transforming HKLs, use "
           "transformHKL.")
      .def("transformHKL", &applyToVector, (arg("self"), arg("hkl")),
           "Returns transformed HKLs. For transformation of coordinates use "
           "transformCoordinates.")
      .def("apply", &applyToVector, (arg("self"), arg("hkl")), "An alias for transformHKL.");

  std_vector_exporter<Mantid::Geometry::SymmetryOperation>::wrap("std_vector_symmetryoperation");
}
