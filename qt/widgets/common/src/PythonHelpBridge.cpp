#include "MantidQtWidgets/Common/PythonHelpBridge.h"
#include <boost/python.hpp>
#include <boost/python/str.hpp>
#include <stdexcept>

namespace { // anonymous
// name of the module to import the bridge
const std::string MOD_NAME("mantidqt.widgets.helpwindow.helpwindowbridge");
// python::str version of the module
const boost::python::str MOD_NAME_PY(MOD_NAME);
} // namespace

PythonHelpBridge::PythonHelpBridge() {
  Py_Initialize();
  try {
    boost::python::object main = boost::python::import("__main__");
    boost::python::object global = main.attr("__dict__");
    boost::python::import(MOD_NAME_PY);
  } catch (boost::python::error_already_set &) {
    PyErr_Print();
    throw std::runtime_error("Failed to import '" + MOD_NAME + "' module.");
  }
}

void PythonHelpBridge::showHelpPage(const std::string &relative_url) {
  try {
    boost::python::object module = boost::python::import(MOD_NAME_PY);
    module.attr("show_help_page")(relative_url);
  } catch (boost::python::error_already_set &) {
    PyErr_Print();
    throw std::runtime_error("Error calling show_help_page in Python.");
  }
}
