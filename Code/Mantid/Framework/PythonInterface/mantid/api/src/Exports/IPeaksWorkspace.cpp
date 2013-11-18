#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>

using Mantid::API::IPeaksWorkspace;
using Mantid::API::IPeaksWorkspace_sptr;
using Mantid::API::IPeak;
using Mantid::API::ExperimentInfo;
using Mantid::API::ITableWorkspace;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

void export_IPeaksWorkspace()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IPeaksWorkspace);

  // IPeaksWorkspace class
  class_< IPeaksWorkspace, bases<ITableWorkspace, ExperimentInfo>, boost::noncopyable >("IPeaksWorkspace", no_init)
    .def("getNumberPeaks", &IPeaksWorkspace::getNumberPeaks, "Returns the number of peaks within the workspace")
    .def("addPeak", &IPeaksWorkspace::addPeak, "Add a peak to the workspace")
    .def("removePeak", &IPeaksWorkspace::removePeak, "Remove a peak from the workspace")
    .def("getPeak", &IPeaksWorkspace::getPeakPtr, return_internal_reference<>(), "Returns a peak at the given index" )
    .def("createPeak", &IPeaksWorkspace::createPeak, return_internal_reference<>(), "Create a Peak and return it")
    .def("hasIntegratedPeaks", &IPeaksWorkspace::hasIntegratedPeaks, "Determine if the peaks have been integrated")
    .def("getRun", &IPeaksWorkspace::mutableRun, return_internal_reference<>(),
             "Return the Run object for this workspace")
    .def("peakInfoNumber", &IPeaksWorkspace::peakInfoNumber, "Peak info number at Q vector for this workspace")
      ;

  REGISTER_SINGLEVALUE_HANDLER(IPeaksWorkspace_sptr);

}

