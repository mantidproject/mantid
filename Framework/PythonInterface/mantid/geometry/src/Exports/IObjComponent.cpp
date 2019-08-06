// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/IObjComponent.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IComponent;
using Mantid::Geometry::IObjComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IObjComponent)

namespace {
/**
 * Returns a non-const pointer to the shape object. Avoids a bug with boost
 * python 1.43
 * that can't register shared_ptr to const types.
 * @param self A reference to the calling object to emulate method on Python
 * object
 */
boost::shared_ptr<Mantid::Geometry::IObject> getShape(IObjComponent &self) {
  return boost::const_pointer_cast<Mantid::Geometry::IObject>(self.shape());
}
} // namespace

void export_IObjComponent() {
  register_ptr_to_python<boost::shared_ptr<IObjComponent>>();

  class_<IObjComponent, boost::python::bases<IComponent>, boost::noncopyable>(
      "IObjComponent", no_init)
      .def("shape", &getShape, arg("self"),
           "Get the object that represents "
           "the physical shape of this "
           "component");
}
