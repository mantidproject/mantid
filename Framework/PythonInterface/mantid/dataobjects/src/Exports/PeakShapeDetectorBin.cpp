// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakShapeDetectorBin.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/tuple.hpp>
#include <vector>

using namespace boost::python;
using namespace Mantid::DataObjects;
using Mantid::Kernel::SpecialCoordinateSystem;
using Mantid::PythonInterface::Converters::PySequenceToVector;

PeakShapeDetectorBin *createPeakShapeDetectorBin(const boost::python::list &pyList, SpecialCoordinateSystem frame,
                                                 const std::string &algorithmName = std::string(),
                                                 int algorithmVersion = -1) {
  // Convert Python list of tuples to std::vector<std::tuple<int32_t, double, double>>
  std::vector<std::tuple<int32_t, double, double>> detectorBinList;

  for (int i = 0; i < boost::python::len(pyList); ++i) {
    boost::python::tuple pyTuple = boost::python::extract<boost::python::tuple>(pyList[i]);
    int32_t detectorID = boost::python::extract<int32_t>(pyTuple[0]);
    double startX = boost::python::extract<double>(pyTuple[1]);
    double endX = boost::python::extract<double>(pyTuple[2]);
    detectorBinList.emplace_back(detectorID, startX, endX);
  }

  return new PeakShapeDetectorBin(detectorBinList, frame, algorithmName, algorithmVersion);
}

void export_PeakShapeDetectorBin() {
  class_<PeakShapeDetectorBin, bases<Mantid::Geometry::PeakShape>, boost::noncopyable>("PeakShapeDetectorBin", no_init)
      .def("__init__", make_constructor(&createPeakShapeDetectorBin, default_call_policies(),
                                        (arg("detectorBinList"), arg("frame") = SpecialCoordinateSystem::None,
                                         arg("algorithmName") = "", arg("algorithmVersion") = -1)));
}
