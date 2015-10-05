
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/kernel/StlExportDefinitions.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::SymmetryOperation;
using Mantid::PythonInterface::std_vector_exporter;

using namespace boost::python;

namespace //<unnamed>
    {
using namespace Mantid::PythonInterface;

Mantid::Kernel::V3D applyToVector(SymmetryOperation &self, const object &hkl) {
  return self.transformHKL(Converters::PyObjectToV3D(hkl)());
}

Mantid::Kernel::V3D applyToCoordinates(SymmetryOperation &self,
                                       const object &coordinates) {
  return self.operator*<Mantid::Kernel::V3D>(
      Converters::PyObjectToV3D(coordinates)());
}
}

void export_SymmetryOperation() {
  register_ptr_to_python<boost::shared_ptr<SymmetryOperation>>();

  class_<SymmetryOperation>("SymmetryOperation")
      .def("getOrder", &SymmetryOperation::order,
           "Returns the order of the symmetry operation, which indicates how "
           "often the operation needs to be applied to a point to arrive at "
           "identity.")
      .def("getIdentifier", &SymmetryOperation::identifier,
           "The identifier of the operation in x,y,z-notation.")
      .def("transformCoordinates", &applyToCoordinates,
           "Returns transformed coordinates. For transforming HKLs, use "
           "transformHKL.")
      .def("transformHKL", &applyToVector, "Returns transformed HKLs. For "
                                           "transformation of coordinates use "
                                           "transformCoordinates.")
      .def("apply", &applyToVector, "An alias for transformHKL.");

  std_vector_exporter<Mantid::Geometry::SymmetryOperation>::wrap(
      "std_vector_symmetryoperation");
}
