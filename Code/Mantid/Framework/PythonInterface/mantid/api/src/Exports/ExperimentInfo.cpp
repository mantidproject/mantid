#include "MantidGeometry/IDTypes.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>

using Mantid::API::ExperimentInfo;
using Mantid::API::ExperimentInfo_sptr;
using namespace boost::python;

/// Overload generator for getInstrumentFilename
BOOST_PYTHON_FUNCTION_OVERLOADS(getInstrumentFilename_Overload, ExperimentInfo::getInstrumentFilename, 1, 2);

void export_ExperimentInfo()
{
  REGISTER_SHARED_PTR_TO_PYTHON(ExperimentInfo);

  class_<ExperimentInfo, boost::noncopyable>("ExperimentInfo", no_init)
          .def("getInstrument", &ExperimentInfo::getInstrument, "Returns the instrument for this run.")

          .def("getInstrumentFilename", &ExperimentInfo::getInstrumentFilename,
               getInstrumentFilename_Overload("Returns IDF",(arg("instrument"),arg("date")="")))
          .staticmethod("getInstrumentFilename")

          .def("sample", &ExperimentInfo::sample, return_value_policy<reference_existing_object>(),
               "Return the Sample object. This cannot be modified, use mutableSample to modify.")

          .def("mutableSample", &ExperimentInfo::mutableSample, return_value_policy<reference_existing_object>(),
               "Return a modifiable Sample object.")

          .def("run", &ExperimentInfo::run, return_value_policy<reference_existing_object>(),
               "Return the sample object. This cannot be modified, use mutableSample to modify.")

          .def("mutableRun", &ExperimentInfo::mutableRun, return_value_policy<reference_existing_object>(),
               "Return a modifiable Run object.")

          .def("getRunNumber", &ExperimentInfo::getRunNumber, "Returns the run identifier for this run.")

          .def("getEFixed", (double (ExperimentInfo::*)(const Mantid::detid_t) const) &ExperimentInfo::getEFixed)

          .def("setEFixed", &ExperimentInfo::setEFixed)

          .def("getEMode", &ExperimentInfo::getEMode, "Returns the energy mode.")
          ;
}
