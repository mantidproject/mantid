#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/ExperimentInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::MultipleExperimentInfos;
using Mantid::API::ExperimentInfo_sptr;
using namespace boost::python;

void export_MultipleExperimentInfos() {
  class_<MultipleExperimentInfos, boost::noncopyable>("MultipleExperimentInfos",
                                                      no_init)
      .def("getExperimentInfo",
           (ExperimentInfo_sptr (MultipleExperimentInfos::*)(const uint16_t)) &
               MultipleExperimentInfos::getExperimentInfo,
           (arg("self"), arg("run_index")),
           "Return the experiment info at the given index.")
      .def("addExperimentInfo", &MultipleExperimentInfos::addExperimentInfo,
           (arg("self"), arg("ExperimentalInfo")),
           "Add a new :class:`~mantid.api.ExperimentInfo` to this "
           ":class:`~mantid.api.IMDWorkspace`")
      .def("getNumExperimentInfo",
           &MultipleExperimentInfos::getNumExperimentInfo, arg("self"),
           "Return the number of :class:`~mantid.api.ExperimentInfo` objects,")
      .def("copyExperimentInfos", &MultipleExperimentInfos::copyExperimentInfos,
           (arg("self"), arg("MultipleExperimentInfos")),
           "Copy the :class:`~mantid.api.ExperimentInfo` from another");
}
