#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/ExperimentInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::MultipleExperimentInfos;
using Mantid::API::ExperimentInfo_sptr;
using namespace boost::python;

// clang-format off
void export_MultipleExperimentInfos()
// clang-format on
{
  class_<MultipleExperimentInfos,boost::noncopyable>("MultipleExperimentInfos", no_init)
      .def("getExperimentInfo", (ExperimentInfo_sptr(MultipleExperimentInfos::*)(const uint16_t) ) &MultipleExperimentInfos::getExperimentInfo,
           "Return the experiment info at the given index.")
      .def("getNumExperimentInfo", &MultipleExperimentInfos::getNumExperimentInfo,
           "Return the number of experiment info objects,")
    ;
}

