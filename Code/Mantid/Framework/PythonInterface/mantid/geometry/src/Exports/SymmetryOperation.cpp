
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::SymmetryOperation;

using namespace boost::python;

namespace //<unnamed>
{
  using namespace Mantid::PythonInterface;

  Mantid::Kernel::V3D applyToVector(SymmetryOperation & self, const object& hkl)
  {
    return self.operator *<Mantid::Kernel::V3D>(Converters::PyObjectToV3D(hkl)());
  }
}

void export_SymmetryOperation()
{
  register_ptr_to_python<boost::shared_ptr<SymmetryOperation> >();

  class_<SymmetryOperation>("SymmetryOperation")
          .def("order", &SymmetryOperation::order)
          .def("identifier", &SymmetryOperation::identifier)
          .def("apply", &applyToVector);
}

