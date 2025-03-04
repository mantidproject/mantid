// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/PythonHelpBridge.h"
#include <Python.h>
#include <boost/python.hpp>
#include <boost/python/str.hpp>
#include <stdexcept>

namespace {
const std::string MOD_NAME("mantidqt.widgets.helpwindow.helpwindowbridge");
boost::python::object getHelpWindowModule() {
  boost::python::object mod = boost::python::import(boost::python::str(MOD_NAME));
  return mod;
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {

PythonHelpBridge::PythonHelpBridge() {}

void PythonHelpBridge::showHelpPage(const std::string &relative_url) {
  PyGILState_STATE gstate = PyGILState_Ensure();
  try {
    auto module = getHelpWindowModule();
    module.attr("show_help_page")(relative_url);
  } catch (boost::python::error_already_set &) {
    PyErr_Print();
    PyGILState_Release(gstate);
    throw std::runtime_error("Error calling show_help_page in Python.");
  }
  PyGILState_Release(gstate);
}
} // namespace MantidWidgets
} // namespace MantidQt
