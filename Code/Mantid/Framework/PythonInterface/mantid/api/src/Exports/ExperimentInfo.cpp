#include "MantidGeometry/IDTypes.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::ExperimentInfo;
using Mantid::PythonInterface::Policies::RemoveConstSharedPtr;
using namespace boost::python;

/// Overload generator for getInstrumentFilename
BOOST_PYTHON_FUNCTION_OVERLOADS(getInstrumentFilename_Overload, ExperimentInfo::getInstrumentFilename, 1, 2)

void export_ExperimentInfo()
{
  register_ptr_to_python<boost::shared_ptr<ExperimentInfo>>();

  class_<ExperimentInfo, boost::noncopyable>("ExperimentInfo", no_init)
          .def("getInstrument", &ExperimentInfo::getInstrument, return_value_policy<RemoveConstSharedPtr>(),
               args("self"), "Returns the instrument for this run.")

          .def("getInstrumentFilename", &ExperimentInfo::getInstrumentFilename,
               getInstrumentFilename_Overload("Returns IDF",(arg("instrument"),arg("date")="")))
          .staticmethod("getInstrumentFilename")

          .def("sample", &ExperimentInfo::sample, return_value_policy<reference_existing_object>(),
               args("self"), "Return the Sample object. This cannot be modified, use mutableSample to modify.")

          .def("mutableSample", &ExperimentInfo::mutableSample, return_value_policy<reference_existing_object>(),
               args("self"), "Return a modifiable Sample object.")

          .def("run", &ExperimentInfo::run, return_value_policy<reference_existing_object>(),
               args("self"), "Return the Run object. This cannot be modified, use mutableRun to modify.")

          .def("mutableRun", &ExperimentInfo::mutableRun, return_value_policy<reference_existing_object>(),
               args("self"), "Return a modifiable Run object.")

          .def("getRunNumber", &ExperimentInfo::getRunNumber, args("self"),
               "Returns the run identifier for this run.")

          .def("getEFixed", (double (ExperimentInfo::*)(const Mantid::detid_t) const) &ExperimentInfo::getEFixed,
               args("self", "detId"))

          .def("setEFixed", &ExperimentInfo::setEFixed, args("self", "detId", "value"))

          .def("getEMode", &ExperimentInfo::getEMode, args("self"), "Returns the energy mode.")
          ;
}
