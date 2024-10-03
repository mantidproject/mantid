// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgoTimeRegister.h"
#include <iostream>
//#include "MantidAPI/AlgorithmExecuteProfile.cpp"
#include "MantidKernel/Timer.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
//#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
//#include "MantidKernel/WarningSuppressions.h"
//#include "MantidAPI/DllConfig.h"

// using namespace Mantid;
using namespace Mantid::Instrumentation;
using namespace boost::python;
using Mantid::Kernel::time_point_ns;

// namespace{
// AlgoTimeRegister AlgoTimeRegister::;
// AlgoTimeRegister AlgoTimeRegister::globalAlgoTimeRegister;

void addTimeWrapper(const std::string &name, long int begin, long int end) {
  // AlgoTimeRegister AlgoTimeRegister;

  time_point_ns chrono_begin = std::chrono::high_resolution_clock::now();
  time_point_ns chrono_end = std::chrono::high_resolution_clock::now();
  std::cout << "name" << name << " begin " << begin << " end " << end << std::endl;
  // std::cout << "self" << self << " chrono_begin" << chrono_begin << " chrono_end " << chrono_end << std::endl;
  // Mantid::Instrumentation::AlgoTimeRegister::globalAlgoTimeRegister.addTime(name, chrono_begin, chrono_end);

  // Mantid::Instrumentation::AlgoTimeRegister::globalAlgoTimeRegister.addTime(name, chrono_begin, chrono_end);
  // self->addTime(name, chrono_begin, chrono_end);
  // return;

  // Create a time point representing a duration in milliseconds
  // std::chrono::milliseconds ms(1234);

  // Convert milliseconds to nanoseconds
  std::chrono::nanoseconds begin_ns(begin); // duration_cast<std::chrono::nanoseconds>(ms);

  // Create a time point from the nanoseconds duration
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_begin_ns(begin_ns);

  // Print the time point
  std::cout << "C++ Begin Time point : " << tp_begin_ns.time_since_epoch().count() << " ns" << std::endl;

  // Convert milliseconds to nanoseconds
  std::chrono::nanoseconds end_ns(end); // duration_cast<std::chrono::nanoseconds>(ms);

  // Create a time point from the nanoseconds duration
  std::chrono::time_point<std::chrono::high_resolution_clock> tp_end_ns(end_ns);

  // Print the time point
  std::cout << "C++ End Time point: " << tp_end_ns.time_since_epoch().count() << " ns" << std::endl;

  // Calculate the duration in nanoseconds
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end_ns - tp_begin_ns);

  std::cout << "C++ Time taken: " << duration.count() << " nanoseconds\n";

  Mantid::Instrumentation::AlgoTimeRegister::Instance().addTime(name, tp_begin_ns, tp_end_ns);
}
//}
// This is typed on the AlgoTimeRegister class
void export_AlgoTimeRegister() {
  register_ptr_to_python<std::shared_ptr<AlgoTimeRegister>>();

  // AlgoTimeRegister class
  class_<AlgoTimeRegister, boost::noncopyable>("AlgoTimeRegister", no_init)
      //.def("test", &AlgoTimeRegister::test, (arg("self"), arg("name")), "Returns the name of the workspace. This could
      // be an empty string")
      .def("addTime", &addTimeWrapper, (arg("name"), arg("begin"), arg("end")));
  //.staticmethod("addTime");
  //.staticmethod("addTime");
}

// thread id=-1 default check with python