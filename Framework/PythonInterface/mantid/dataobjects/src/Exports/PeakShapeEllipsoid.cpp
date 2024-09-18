// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>

using namespace boost::python;
using namespace Mantid::DataObjects;
using Mantid::Kernel::SpecialCoordinateSystem;
using Mantid::Kernel::V3D;
using Mantid::PythonInterface::Converters::PySequenceToVector;

PeakShapeEllipsoid *initPeakShapeEllipsoid(const boost::python::list &directions, const boost::python::list &abcRadii,
                                           const boost::python::list &abcRadiiBackgroundInner,
                                           const boost::python::list abcRadiiBackgroundOuter,
                                           SpecialCoordinateSystem frame) {
  return new PeakShapeEllipsoid(PySequenceToVector<V3D>(directions)(), PySequenceToVector<double>(abcRadii)(),
                                PySequenceToVector<double>(abcRadiiBackgroundInner)(),
                                PySequenceToVector<double>(abcRadiiBackgroundOuter)(), frame);
}

void export_PeakShapeEllipsoid() {
  class_<PeakShapeEllipsoid, bases<Mantid::Geometry::PeakShape>, boost::noncopyable>("PeakShapeEllipsoid", no_init)
      .def("__init__",
           make_constructor(&initPeakShapeEllipsoid, default_call_policies(),
                            (arg("directions"), arg("abcRadii"), arg("abcRadiiBackgroundInner"),
                             arg("abcRadiiBackgroundOuter"), arg("frame") = SpecialCoordinateSystem::QSample)));
}
