#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/optional.hpp>
#include <boost/python/manage_new_object.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::API;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IPeaksWorkspace)

namespace {

/// Create a peak via it's HKL value from a list or numpy array
IPeak *createPeakHKL(IPeaksWorkspace &self, const object &data) {
  return self.createPeakHKL(
      Mantid::PythonInterface::Converters::PyObjectToV3D(data)());
}

/// Create a peak via it's QLab value from a list or numpy array
IPeak *createPeakQLab(IPeaksWorkspace &self, const object &data) {
  return self.createPeak(
      Mantid::PythonInterface::Converters::PyObjectToV3D(data)(), boost::none);
}

/// Create a peak via it's QLab value from a list or numpy array
IPeak *createPeakQLabWithDistance(IPeaksWorkspace &self, const object &data,
                                  double detectorDistance) {
  return self.createPeak(
      Mantid::PythonInterface::Converters::PyObjectToV3D(data)(),
      detectorDistance);
}
}

void export_IPeaksWorkspace() {
  // IPeaksWorkspace class
  class_<IPeaksWorkspace, bases<ITableWorkspace, ExperimentInfo>,
         boost::noncopyable>("IPeaksWorkspace", no_init)
      .def("getNumberPeaks", &IPeaksWorkspace::getNumberPeaks, arg("self"),
           "Returns the number of peaks within the workspace")
      .def("addPeak", &IPeaksWorkspace::addPeak, (arg("self"), arg("peak")),
           "Add a peak to the workspace")
      .def("removePeak", &IPeaksWorkspace::removePeak,
           (arg("self"), arg("peak_num")), "Remove a peak from the workspace")
      .def("getPeak", &IPeaksWorkspace::getPeakPtr,
           (arg("self"), arg("peak_num")), return_internal_reference<>(),
           "Returns a peak at the given index")
      .def("createPeak", createPeakQLab, (arg("self"), arg("data")),
           return_value_policy<manage_new_object>(),
           "Create a Peak and return it from its coordinates in the QLab frame")
      .def("createPeak", createPeakQLabWithDistance,
           (arg("self"), arg("data"), arg("detector_distance")),
           return_value_policy<manage_new_object>(),
           "Create a Peak and return it from its coordinates in the QLab "
           "frame, detector-sample distance explicitly provided")
      .def("createPeakHKL", createPeakHKL, (arg("self"), arg("data")),
           return_value_policy<manage_new_object>(),
           "Create a Peak and return it from its coordinates in the HKL frame")
      .def("hasIntegratedPeaks", &IPeaksWorkspace::hasIntegratedPeaks,
           arg("self"), "Determine if the peaks have been integrated")
      .def("getRun", &IPeaksWorkspace::mutableRun, arg("self"),
           return_internal_reference<>(),
           "Return the Run object for this workspace")
      .def("peakInfoNumber", &IPeaksWorkspace::peakInfoNumber,
           (arg("self"), arg("qlab_frame"), arg("lab_coordinate")),
           "Peak info number at Q vector for this workspace");

  //-------------------------------------------------------------------------------------------------

  RegisterWorkspacePtrToPython<IPeaksWorkspace>();
}
