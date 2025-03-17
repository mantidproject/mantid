// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ProgressBase.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::ProgressBase;
using namespace boost::python;

void export_ProgressBase() {
  class_<ProgressBase, boost::noncopyable>("ProgressBase", no_init)
      .def("report", (void (ProgressBase::*)())&ProgressBase::report, arg("self"),
           "Increment the progress by 1 and report with no message")

      .def("report", (void (ProgressBase::*)(const std::string &))&ProgressBase::report, (arg("self"), arg("msg")),
           "Increment the progress by 1 and report along with "
           "the given message")

      .def("report", (void (ProgressBase::*)(int64_t, const std::string &))&ProgressBase::report,
           (arg("self"), arg("i"), arg("msg")),
           "Set the progress to given amount and "
           "report along with the given message")

      .def("reportIncrement", (void (ProgressBase::*)(size_t, const std::string &))&ProgressBase::reportIncrement,
           (arg("self"), arg("i"), arg("msg")),
           "Increment the progress by given amount and "
           "report along with the given message")

      .def("setNumSteps", &ProgressBase::setNumSteps, (arg("self"), arg("nsteps")),
           "Sets a new number of steps for the current progress range")

      .def("resetNumSteps", &ProgressBase::resetNumSteps, (arg("self"), arg("nsteps"), arg("start"), arg("end")),
           "Resets the number of steps & progress range to the given values")

      .def("setNotifyStep", &ProgressBase::setNotifyStep, (arg("self"), arg("notifyStep")),
           "Set how often the notifications are actually reported")

      .def("getEstimatedTime", &ProgressBase::getEstimatedTime, arg("self"),
           "Returns an estimate of the time remaining. May not be to accurate "
           "if the reporting is lumpy.");
}
