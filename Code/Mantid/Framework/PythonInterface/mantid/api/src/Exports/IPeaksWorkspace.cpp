#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/optional.hpp>
#include <boost/python/manage_new_object.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::DataItemInterface;
using namespace boost::python;

namespace {

/// Create a peak via it's HKL value from a list or numpy array
IPeak* createPeakHKL(IPeaksWorkspace & self, const object& data)
{
  return self.createPeakHKL(Mantid::PythonInterface::Converters::PyObjectToV3D(data)());
}

/// Create a peak via it's QLab value from a list or numpy array
IPeak* createPeakQLab(IPeaksWorkspace & self, const object& data)
{
  return self.createPeak(Mantid::PythonInterface::Converters::PyObjectToV3D(data)(), boost::optional<double>());
}

/// Create a peak via it's QLab value from a list or numpy array
IPeak* createPeakQLabWithDistance(IPeaksWorkspace & self, const object& data, double detectorDistance)
{
  return self.createPeak(Mantid::PythonInterface::Converters::PyObjectToV3D(data)(), detectorDistance);
}

}

void export_IPeaksWorkspace()
{
  // IPeaksWorkspace class
  class_< IPeaksWorkspace, bases<ITableWorkspace, ExperimentInfo>, boost::noncopyable >("IPeaksWorkspace", no_init)
    .def("getNumberPeaks", &IPeaksWorkspace::getNumberPeaks, "Returns the number of peaks within the workspace")
    .def("addPeak", &IPeaksWorkspace::addPeak, "Add a peak to the workspace")
    .def("removePeak", &IPeaksWorkspace::removePeak, "Remove a peak from the workspace")
    .def("getPeak", &IPeaksWorkspace::getPeakPtr, return_internal_reference<>(), "Returns a peak at the given index" )
    .def("createPeak", createPeakQLab, return_value_policy<manage_new_object>(), "Create a Peak and return it from its coordinates in the QLab frame")
    .def("createPeak", createPeakQLabWithDistance, return_value_policy<manage_new_object>(), "Create a Peak and return it from its coordinates in the QLab frame, detector-sample distance explicitly provided")
    .def("createPeakHKL", createPeakHKL, return_value_policy<manage_new_object>(), "Create a Peak and return it from its coordinates in the HKL frame")
    .def("hasIntegratedPeaks", &IPeaksWorkspace::hasIntegratedPeaks, "Determine if the peaks have been integrated")
    .def("getRun", &IPeaksWorkspace::mutableRun, return_internal_reference<>(),
             "Return the Run object for this workspace")
    .def("peakInfoNumber", &IPeaksWorkspace::peakInfoNumber, "Peak info number at Q vector for this workspace")
      ;

  //-------------------------------------------------------------------------------------------------

  DataItemInterface<IPeaksWorkspace>()
    .castFromID("PeaksWorkspace")
  ;
}

