#include "MantidKernel/ProgressBase.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::ProgressBase;
using namespace boost::python;

// clang-format off
void export_ProgressBase()
// clang-format on
{
  class_<ProgressBase,boost::noncopyable>("ProgressBase", no_init)
    .def("report", (void (ProgressBase::*)())&ProgressBase::report, "Increment the progress by 1 and report with no message")

    .def("report", (void (ProgressBase::*)(const std::string&))&ProgressBase::report, (arg("msg")),
         "Increment the progress by 1 and report along with the given message")

    .def("report", (void (ProgressBase::*)(int64_t,const std::string&))&ProgressBase::report, (arg("i"),arg("msg")),
         "Set the progress to given amount and report along with the given message")

    .def("reportIncrement", (void (ProgressBase::*)(size_t,const std::string&))&ProgressBase::reportIncrement, (arg("i"),arg("msg")),
         "Increment the progress by given amount and report along with the given message")

    .def("setNumSteps", &ProgressBase::setNumSteps, (arg("nsteps")),
         "Sets a new number of steps for the current progress range")

    .def("resetNumSteps", &ProgressBase::resetNumSteps, (arg("nsteps"),arg("start"),arg("end")),
         "Resets the number of steps & progress range to the given values")

    .def("setNotifyStep", &ProgressBase::setNotifyStep, (arg("notifyStep")),
         "Set how often the notifications are actually reported")

    .def("getEstimatedTime", &ProgressBase::getEstimatedTime,
         "Returns an estimate of the time remaining. May not be to accurate if the reporting is lumpy.")
    ;
}

