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

#include <iostream>

// using namespace Mantid;
using namespace Mantid::Instrumentation;
using namespace boost::python;
using Mantid::Kernel::time_point_ns;

// this works only in linux
#ifdef __linux__
void addTimeWrapper(const std::string &name, long int begin, long int end) {

  std::chrono::nanoseconds begin_ns(begin);
  std::chrono::nanoseconds end_ns(end);

  // add the duration to the start time point
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_begin_ns(begin_ns);

  // Print the time point
  std::cout << "C++ Begin Time point : " << tp_begin_ns.time_since_epoch().count() << " ns" << std::endl;

  // add the duration to the end time point
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_end_ns(end_ns);

  // Print the time point
  std::cout << "C++ End Time point: " << tp_end_ns.time_since_epoch().count() << " ns" << std::endl;

  // Calculate the duration in nanoseconds
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end_ns - tp_begin_ns);

  std::cout << "C++ Time taken: " << duration.count() << " nanoseconds\n";
  // std::cout << "C++ Time: " << start.time_since_epoch().count() << " \n";

  Mantid::Instrumentation::AlgoTimeRegister::Instance().addTime(name, tp_begin_ns, tp_end_ns);
}

AlgoTimeRegisterImpl &start() {
  auto &svc = AlgoTimeRegister::Instance();
  return svc;
}

void export_AlgoTimeRegister() {

  // AlgoTimeRegister class
  class_<AlgoTimeRegisterImpl, boost::noncopyable>("AlgoTimeRegisterImpl", no_init)
      .def("addTime", &addTimeWrapper, (arg("name"), arg("begin"), arg("end")))
      .staticmethod("addTime")
      .def("start", start, return_value_policy<reference_existing_object>(),
           "Returns a reference to the AlgoTimeRegister")
      .staticmethod("start")
      .def("Instance", &AlgoTimeRegister::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the AlgoTimeRegister")
      .staticmethod("Instance");
}
#endif