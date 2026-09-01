// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/IObject.h"
#include "MantidPythonInterface/core/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/MatrixToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IObject;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IObject)

void export_IObject() {
  register_ptr_to_python<std::shared_ptr<IObject>>();

  using return_readonly_numpy = return_value_policy<Policies::MatrixRefToNumpy<Converters::WrapReadOnly>>;

  class_<IObject, boost::noncopyable>("IObject", no_init)
      .def("getAppliedRotation", &IObject::getAppliedRotation, arg("self"), return_readonly_numpy(),
           "Return the goniometer rotation baked into this shape.\n\n"
           "This reports which frame the shape is in, not every rotation it has ever had. "
           "Definition-frame rotations - the file-load orientation of :ref:`algm-LoadSampleShape`, "
           "the sample environment spec, 'rotate-all' and per-primitive 'rotate' tags, and "
           ":ref:`algm-RotateSampleShape` - re-express the shape within its own frame and are "
           "excluded. The identity therefore means the shape is expressed in its own frame, however "
           "much its definition has been rotated within that frame.");
}
