// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Logger.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <memory>

using Mantid::Kernel::Logger;
using namespace boost::python;
using LoggerMsgFunction = void (Logger::*)(const std::string &);
using LoggerFlushFunction = void (Logger::*)();

namespace {
/**
 *
 * @param name The name for the logger instance
 * @return A new logger instance
 */
std::shared_ptr<Logger> getLogger(const std::string &name) {
  PyErr_Warn(PyExc_DeprecationWarning, "Logger.get(\"name\") is deprecated. "
                                       "Simply use Logger(\"name\") instead");
  return std::make_shared<Logger>(name);
}

std::shared_ptr<Logger> create(const std::string &name) { return std::make_shared<Logger>(name); }
} // namespace

void export_Logger() {
  register_ptr_to_python<std::shared_ptr<Logger>>();

  class_<Logger, boost::noncopyable>("Logger", init<std::string>((arg("self"), arg("name"))))
      .def("__init__", make_constructor(&create, default_call_policies(), args("name")))
      .def("fatal", (LoggerMsgFunction)&Logger::fatal, (arg("self"), arg("message")),
           "Send a message at fatal priority: "
           "An unrecoverable error has occured and the application will "
           "terminate")
      .def("error", (LoggerMsgFunction)&Logger::error, (arg("self"), arg("message")),
           "Send a message at error priority: "
           "An error has occured but the framework is able to handle it and "
           "continue")
      .def("warning", (LoggerMsgFunction)&Logger::warning, (arg("self"), arg("message")),
           "Send a message at warning priority: "
           "Something was wrong but the framework was able to continue despite "
           "the problem.")
      .def("notice", (LoggerMsgFunction)&Logger::notice, (arg("self"), arg("message")),
           "Sends a message at notice priority: "
           "Really important information that should be displayed to the user, "
           "this should be minimal. The default logging level is set here "
           "unless it is altered.")
      .def("information", (LoggerMsgFunction)&Logger::information, (arg("self"), arg("message")),
           "Send a message at information priority: "
           "Useful but not vital information to be relayed back to the user.")
      .def("debug", (LoggerMsgFunction)&Logger::debug, (arg("self"), arg("message")),
           "Send a message at debug priority:"
           ". Anything that may be useful to understand what the code has been "
           "doing for debugging purposes.")
      .def("accumulate", (LoggerMsgFunction)&Logger::accumulate, (arg("self"), arg("message")),
           "accumulate a message to report later")
      .def("flush", (LoggerFlushFunction)&Logger::flush, (arg("self")),
           "Flush the accumulated message to the current channel.")
      .def("flushDebug", (LoggerFlushFunction)&Logger::flushDebug, (arg("self")),
           "Flush the accumulated message to the debug channel.")
      .def("flushInformation", (LoggerFlushFunction)&Logger::flushInformation, (arg("self")),
           "Flush the accumulated message to the debug channel.")
      .def("flushNotice", (LoggerFlushFunction)&Logger::flushNotice, (arg("self")),
           "Flush the accumulated message to the notice channel.")
      .def("flushWarning", (LoggerFlushFunction)&Logger::flushWarning, (arg("self")),
           "Flush the accumulated message to the warning channel.")
      .def("flushError", (LoggerFlushFunction)&Logger::flushError, (arg("self")),
           "Flush the accumulated message to the error channel.")
      .def("flushFatal", (LoggerFlushFunction)&Logger::flushFatal, (arg("self")),
           "Flush the accumulated message to the fatal channel.")
      .def("purge", (LoggerFlushFunction)&Logger::purge, (arg("self")),
           "Clear the accumulated messages without logging.")
      // -- deprecated  --
      .def("get", &getLogger,
           "Creates the named logger. "
           "This method is static, call as Logger.get('logger_name'). The name "
           "is used as a prefix within the "
           "log file so that msg origins can be traced more easily.")
      .staticmethod("get");
}
