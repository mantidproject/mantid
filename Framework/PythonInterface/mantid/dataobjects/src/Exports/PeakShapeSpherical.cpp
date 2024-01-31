// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeSpherical.h"
#include <boost/python/class.hpp>

using namespace boost::python;
using namespace Mantid::DataObjects;
using Mantid::Kernel::SpecialCoordinateSystem;

void export_PeakShapeSpherical() {

  class_<PeakShapeSpherical, bases<Mantid::Geometry::PeakShape>, boost::noncopyable>("PeakShapeSpherical", no_init)
      .def(init<const double &, SpecialCoordinateSystem>(
          (arg("peakRadius"), arg("frame") = SpecialCoordinateSystem::QSample)))
      .def(init<const double &, const double &, const double &, SpecialCoordinateSystem>(
          (arg("peakRadius"), arg("backgroundInnerRadius"), arg("backgroundOuterRadius"),
           arg("frame") = SpecialCoordinateSystem::QSample)));
}
