// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Logger.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToString.h"
#include <boost/make_shared.hpp>
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::Logger;
using namespace boost::python;
namespace Converters = Mantid::PythonInterface::Converters;

namespace {
/**
 *
 * @param name The name for the logger instance
 * @return A new logger instance
 */
boost::shared_ptr<Logger> getLogger(const std::string &name) {
  PyErr_Warn(PyExc_DeprecationWarning, "Logger.get(\"name\") is deprecated. "
                                       "Simply use Logger(\"name\") instead");
  return boost::make_shared<Logger>(name);
}

boost::shared_ptr<Logger> create(const boost::python::object &name) {
  return boost::make_shared<Logger>(Converters::pyObjToStr(name));
}

void fatal(Logger *self, const boost::python::object &message) {
  self->fatal(Converters::pyObjToStr(message));
}

void error(Logger *self, const boost::python::object &message) {
  self->error(Converters::pyObjToStr(message));
}

void warning(Logger *self, const boost::python::object &message) {
  self->warning(Converters::pyObjToStr(message));
}

void notice(Logger *self, const boost::python::object &message) {
  self->notice(Converters::pyObjToStr(message));
}

void information(Logger *self, const boost::python::object &message) {
  self->information(Converters::pyObjToStr(message));
}

void debug(Logger *self, const boost::python::object &message) {
  self->debug(Converters::pyObjToStr(message));
}
} // namespace

void export_Logger() {
  register_ptr_to_python<boost::shared_ptr<Logger>>();

  class_<Logger, boost::noncopyable>(
      "Logger", init<std::string>((arg("self"), arg("name"))))
      .def("__init__",
           make_constructor(&create, default_call_policies(), args("name")))
      .def("fatal", &fatal, (arg("self"), arg("message")),
           "Send a message at fatal priority: "
           "An unrecoverable error has occured and the application will "
           "terminate")
      .def("error", &error, (arg("self"), arg("message")),
           "Send a message at error priority: "
           "An error has occured but the framework is able to handle it and "
           "continue")
      .def("warning", &warning, (arg("self"), arg("message")),
           "Send a message at warning priority: "
           "Something was wrong but the framework was able to continue despite "
           "the problem.")
      .def("notice", &notice, (arg("self"), arg("message")),
           "Sends a message at notice priority: "
           "Really important information that should be displayed to the user, "
           "this should be minimal. The default logging level is set here "
           "unless it is altered.")
      .def("information", &information, (arg("self"), arg("message")),
           "Send a message at information priority: "
           "Useful but not vital information to be relayed back to the user.")
      .def("debug", &debug, (arg("self"), arg("message")),
           "Send a message at debug priority:"
           ". Anything that may be useful to understand what the code has been "
           "doing for debugging purposes.")
      // -- deprecated  --
      .def("get", &getLogger,
           "Creates the named logger. "
           "This method is static, call as Logger.get('logger_name'). The name "
           "is used as a prefix within the "
           "log file so that msg origins can be traced more easily.")
      .staticmethod("get");
}
