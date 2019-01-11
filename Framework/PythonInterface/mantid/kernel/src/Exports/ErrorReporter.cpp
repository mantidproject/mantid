// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ErrorReporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::Kernel::ErrorReporter;
using namespace boost::python;

void export_ErrorReporter() {

  class_<ErrorReporter>("ErrorReporter",
                        init<std::string, Mantid::Types::Core::time_duration,
                             std::string, bool>())
      .def(init<std::string, Mantid::Types::Core::time_duration, std::string,
                bool, std::string, std::string, std::string>())

      .def(init<std::string, Mantid::Types::Core::time_duration, std::string,
                bool, std::string, std::string, std::string, std::string>())

      .def("sendErrorReport", &ErrorReporter::sendErrorReport, arg("self"),
           "Sends an error report");
}
