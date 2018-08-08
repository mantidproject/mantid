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
                bool, std::string, std::string>())

      .def("sendErrorReport", &ErrorReporter::sendErrorReport, arg("self"),
           "Sends an error report");
}
