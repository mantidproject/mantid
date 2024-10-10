// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgoTimeRegister.h"
#include "MantidKernel/Timer.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace Mantid::Instrumentation;
using namespace boost::python;
using Mantid::Kernel::time_point_ns;

// this works only in linux
#ifdef __linux__

void addTimeWrapper(const std::string &name, long int begin, long int end) {

  std::chrono::nanoseconds begin_ns(begin);
  std::chrono::nanoseconds end_ns(end);

  // calculate the start time timepoint
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_begin_ns(begin_ns);

  // calculate the end time timepoint
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_end_ns(end_ns);

  Mantid::Instrumentation::AlgoTimeRegister::Instance().addTime(name, tp_begin_ns, tp_end_ns);
}

void export_AlgoTimeRegister() {

  // AlgoTimeRegister class
  class_<AlgoTimeRegisterImpl, boost::noncopyable>("AlgoTimeRegisterImpl", no_init)
      .def("addTime", &addTimeWrapper, (arg("name"), arg("begin"), arg("end")),
           "Adds a time entry in the file for a function with <name> that starts at <begin> time_ns and ends at <end> "
           "time_ns relative to the <START_POINT> clock")
      .staticmethod("addTime")
      .def("Instance", &AlgoTimeRegister::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the AlgoTimeRegister")
      .staticmethod("Instance");
}

#endif