#include "MantidKernel/Logger.h"
#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::Kernel::Logger;
using namespace boost::python;

void export_Logger()
{
  // To distinguish between the overloaded functions
  typedef void (Logger::*LogLevelFunction)(const std::string &);

  class_<Logger,boost::noncopyable>("Logger", no_init)
    .def("fatal", (LogLevelFunction)&Logger::fatal, "Send a message at fatal priority: "
        "An unrecoverable error has occured and the application will terminate")
    .def("error", (LogLevelFunction)&Logger::error, "Send a message at error priority: "
        "An error has occured but the framework is able to handle it and continue")
    .def("warning", (LogLevelFunction)&Logger::warning, "Send a message at warning priority: "
        "Something was wrong but the framework was able to continue despite the problem.")
    .def("notice", (LogLevelFunction)&Logger::notice, "Sends a message at notice priority: "
        "Really important information that should be displayed to the user, "
        "this should be minimal. The default logging level is set here unless it is altered.")
    .def("information", (LogLevelFunction)&Logger::information, "Send a message at information priority: "
        "Useful but not vital information to be relayed back to the user.")
    .def("debug", (LogLevelFunction)&Logger::debug, "Send a message at debug priority:"
        ". Anything that may be useful to understand what the code has been doing for debugging purposes.")
    //--------------- Loggers are created by a static factory function on the class -------------------------
    .def("get", &Logger::get, return_value_policy<reference_existing_object>(), "Creates the named logger. "
        "This method is static, call as Logger.get('logger_name'). The name is used as a prefix within the "
        "log file so that msg origins can be traced more easily.")
    .staticmethod("get")
     ;


}

