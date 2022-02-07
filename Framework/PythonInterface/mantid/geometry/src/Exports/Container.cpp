// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Container.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::Container;
using Mantid::Geometry::IObject;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Container)

void export_Container() {
  register_ptr_to_python<std::shared_ptr<Container>>();

  class_<Container, boost::python::bases<IObject>, boost::noncopyable>("Container", no_init)
      .def("getShape", &Container::getShape, arg("self"), "Returns a shape of a Sample object.",
           return_value_policy<reference_existing_object>());
}
