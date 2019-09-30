// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/IDetector.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IDetector;
using Mantid::Geometry::IObjComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IDetector)

void export_IDetector() {
  register_ptr_to_python<boost::shared_ptr<IDetector>>();
  register_ptr_to_python<boost::shared_ptr<const IDetector>>();

  class_<IDetector, bases<IObjComponent>, boost::noncopyable>("IDetector",
                                                              no_init)
      .def("getID", &IDetector::getID, arg("self"), "Returns the detector ID")
      .def("solidAngle", &IDetector::solidAngle, (arg("self"), arg("observer")),
           "Return the solid angle in steradians between this "
           "detector and an observer")
      .def("getTwoTheta", &IDetector::getTwoTheta,
           (arg("self"), arg("observer"), arg("axis")),
           "Calculate the angle between this detector, another component and "
           "an axis")
      .def("getPhi", &IDetector::getPhi, arg("self"),
           "Returns the azimuthal angle of this detector");
}
