#include "MantidAPI/ExperimentInfo.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::API::ExperimentInfo;
using namespace boost::python;

void export_ExperimentInfo()
{

  class_<ExperimentInfo, boost::noncopyable>("ExperimentInfo", no_init)
    .def("getInstrument", &ExperimentInfo::getInstrument, "Returns the instrument for this run")
    .def("sample", &ExperimentInfo::sample, return_value_policy<reference_existing_object>(),
         "Return the Sample object. This cannot be modified, use mutableSample to modify.")
    .def("mutableSample", &ExperimentInfo::mutableSample, return_value_policy<reference_existing_object>(),
         "Return a modifiable Sample object.")
    .def("run", &ExperimentInfo::run, return_value_policy<reference_existing_object>(),
         "Return the sample object. This cannot be modified, use mutableSample to modify.")
    .def("mutableRun", &ExperimentInfo::mutableRun, return_value_policy<reference_existing_object>(),
         "Return a modifiable Run object.")
    .def("getRunNumber", &ExperimentInfo::getRunNumber, "Returns the run identifier for this run")
    ;
}

