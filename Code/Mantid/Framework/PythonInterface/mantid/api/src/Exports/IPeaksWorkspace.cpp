#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::DataItemInterface;
using namespace boost::python;

namespace {

/// Create a peak via it's HKL value from a list or numpy array
void createPeakHKL(IPeaksWorkspace & self, const object& data)
{
  self.createPeakHKL(Mantid::PythonInterface::Converters(data));
}

/// Create a peak via it's QLab value from a list or numpy array
void createPeakQLab(IPeaksWorkspace & self, const object& data)
{
  self.createPeak(Mantid::PythonInterface::Converters(data));
}

/// Create a peak via it's QLab value from a list or numpy array
void createPeakQLabWithDistance(IPeaksWorkspace & self, const object& data, double detectorDistance)
{
  self.createPeak(Mantid::PythonInterface::Converters(data), distance);
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
    .def("createPeak", createPeakQLab, return_internal_reference<>(), "Create a Peak and return it from its coordinates in the QLab frame")
    .def("createPeak", createPeakQLabWithDistance, return_internal_reference<>(), "Create a Peak and return it from its coordinates in the QLab frame, detector-sample distance explicitly provided")
    .def("createPeakHKL", createPeakHKL, return_internal_reference<>(), "Create a Peak and return it from its coordinates in the HKL frame")
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

