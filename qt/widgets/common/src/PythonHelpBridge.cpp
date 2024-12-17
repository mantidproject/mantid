#include "PythonHelpBridge.h"
#include <boost/python.hpp>
#include <stdexcept>

PythonHelpBridge::PythonHelpBridge() {
  Py_Initialize();
  try {
    boost::python::object main = boost::python::import("__main__");
    boost::python::object global = main.attr("__dict__");
    boost::python::import("helpwindowbridge");
  } catch (boost::python::error_already_set &) {
    PyErr_Print();
    throw std::runtime_error("Failed to import 'helpwindowbridge' Python module.");
  }
}

void PythonHelpBridge::showHelpPage(const std::string &relative_url) {
  try {
    boost::python::object module = boost::python::import("helpwindowbridge");
    module.attr("show_help_page")(relative_url);
  } catch (boost::python::error_already_set &) {
    PyErr_Print();
    throw std::runtime_error("Error calling show_help_page in Python.");
  }
}
