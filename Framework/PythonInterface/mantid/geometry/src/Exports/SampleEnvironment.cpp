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
#include <boost/python/return_internal_reference.hpp>

using Mantid::Geometry::SampleEnvironment;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SampleEnvironment)

void export_SampleEnvironment() {
  register_ptr_to_python<std::shared_ptr<SampleEnvironment>>();

  class_<SampleEnvironment>("SampleEnvironment", no_init)
      .def("getContainer", &SampleEnvironment::getContainer, (args("self")), "Returns the Container or Can",
           return_internal_reference<>())
      .def("getComponent", &SampleEnvironment::getComponent, (args("self", "index")),
           "Returns the requested IObject. Index 0 is considered the container", return_internal_reference<>())
      .def("nelements", &SampleEnvironment::nelements, (args("self")),
           "Returns the number of elements the environment is composed of.")
      .def("isOnGoniometer", &SampleEnvironment::isOnGoniometer, (args("self", "index")),
           "Whether the element turns with the sample when a goniometer is set. Index 0 is the container");
}
