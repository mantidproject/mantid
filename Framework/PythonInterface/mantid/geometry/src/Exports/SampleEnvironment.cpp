// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::SampleEnvironment;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SampleEnvironment)

void export_SampleEnvironment() {
  register_ptr_to_python<std::shared_ptr<SampleEnvironment>>();

  class_<SampleEnvironment>("SampleEnvironment", no_init)
      .def("getContainer", &SampleEnvironment::getContainer, (args("self")), "Returns the Container or Can",
           return_value_policy<reference_existing_object>())
      .def("getComponent", &SampleEnvironment::getComponent, (args("self", "index")),
           "Returns the requested IObject. Index 0 is considered the container",
           return_value_policy<reference_existing_object>());
}
