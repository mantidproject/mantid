// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SetValueWhenProperty.h"
#include "MantidPythonInterface/core/StdFunctionExporter.h"
#include <boost/python.hpp>
#include <functional>
#include <string>

using Mantid::PythonInterface::std_function_from_python;

using namespace Mantid::Kernel;
using namespace boost::python;

void export_SetValueWhenProperty() {

  std_function_from_python<std::string, std::string, std::string>(); // register converter

  class_<SetValueWhenProperty, bases<IPropertySettings>>("SetValueWhenProperty", no_init) // no default constructor

      .def(init<std::string, std::function<std::string(std::string, std::string)>>(
          (arg("self"), arg("watchedPropName"), arg("changeCriterion")),
          "Set value of property when watched-property value criterion is satisfied"));
}
