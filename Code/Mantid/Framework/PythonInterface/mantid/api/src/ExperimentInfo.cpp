#include "MantidAPI/ExperimentInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::ExperimentInfo;
using boost::python::class_;
using boost::python::no_init;

void export_ExperimentInfo()
{
  class_<ExperimentInfo,boost::noncopyable>("ExperimentInfo", no_init)
    .def("getRunNumber", &ExperimentInfo::getRunNumber, "Returns the run identifier for this run")
    ;
}

