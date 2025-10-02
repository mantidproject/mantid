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
#include <stdexcept>

using namespace boost::python;
using namespace Mantid::DataObjects;
using Mantid::Kernel::SpecialCoordinateSystem;
using Mantid::Kernel::V3D;
using Mantid::PythonInterface::Converters::PySequenceToVector;

PeakShapeEllipsoid *initPeakShapeEllipsoid(const boost::python::list &directions, const boost::python::list &abcRadii,
                                           const boost::python::list &abcRadiiBackgroundInner,
                                           const boost::python::list abcRadiiBackgroundOuter,
                                           SpecialCoordinateSystem frame) {
  constexpr size_t ARRAY_SIZE{3}; // PeakShapeEllipsoid only takes array<double,3>

  auto directionsVec = PySequenceToVector<V3D>(directions)();
  if (directionsVec.size() != ARRAY_SIZE) {
    throw std::runtime_error("directions must be size=3. Found size=" + std::to_string(directionsVec.size()));
  }
  std::array<V3D, ARRAY_SIZE> directionsArray = {directionsVec[0], directionsVec[1], directionsVec[2]};

  auto abcRadiiVec = PySequenceToVector<double>(abcRadii)();
  if (abcRadiiVec.size() != ARRAY_SIZE) {
    throw std::runtime_error("abcRadii must be size=3. Found size=" + std::to_string(abcRadiiVec.size()));
  }
  std::array<double, ARRAY_SIZE> abcRadiiArray = {abcRadiiVec[0], abcRadiiVec[1], abcRadiiVec[2]};

  auto abcRadiiBackgroundInnerVec = PySequenceToVector<double>(abcRadiiBackgroundInner)();
  if (abcRadiiBackgroundInnerVec.size() != ARRAY_SIZE) {
    throw std::runtime_error("abcRadiiBackgroundInner must be size=3. Found size=" +
                             std::to_string(abcRadiiBackgroundInnerVec.size()));
  }
  std::array<double, ARRAY_SIZE> abcRadiiBackgroundInnerArray = {
      abcRadiiBackgroundInnerVec[0], abcRadiiBackgroundInnerVec[1], abcRadiiBackgroundInnerVec[2]};

  auto abcRadiiBackgroundOuterVec = PySequenceToVector<double>(abcRadiiBackgroundOuter)();
  if (abcRadiiBackgroundOuterVec.size() != ARRAY_SIZE) {
    throw std::runtime_error("abcRadiiBackgroundOuter must be size=3. Found size=" +
                             std::to_string(abcRadiiBackgroundOuterVec.size()));
  }
  std::array<double, ARRAY_SIZE> abcRadiiBackgroundOuterArray = {
      abcRadiiBackgroundOuterVec[0], abcRadiiBackgroundOuterVec[1], abcRadiiBackgroundOuterVec[2]};

  // create the object
  return new PeakShapeEllipsoid(directionsArray, abcRadiiArray, abcRadiiBackgroundInnerArray,
                                abcRadiiBackgroundOuterArray, frame);
}

void export_PeakShapeEllipsoid() {
  class_<PeakShapeEllipsoid, bases<Mantid::Geometry::PeakShape>, boost::noncopyable>("PeakShapeEllipsoid", no_init)
      .def("__init__",
           make_constructor(&initPeakShapeEllipsoid, default_call_policies(),
                            (arg("directions"), arg("abcRadii"), arg("abcRadiiBackgroundInner"),
                             arg("abcRadiiBackgroundOuter"), arg("frame") = SpecialCoordinateSystem::QSample)));
}
