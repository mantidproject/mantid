// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SetDefaultWhenProperty.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/Property.h"
#include "MantidPythonInterface/core/StdFunctionExporter.h"

#include <boost/python.hpp>
// #include <boost/python/converter/registry.hpp>
#include <functional>
#include <string>
#include <type_traits>

using Mantid::PythonInterface::std_function_from_python;

using namespace Mantid::Kernel;
using namespace boost::python;

void export_SetDefaultWhenProperty() {

  std_function_from_python<bool, const Mantid::Kernel::IPropertyManager *, Property *,
                           Property *>(); // register converter

  class_<SetDefaultWhenProperty, bases<IPropertySettings>>("SetDefaultWhenProperty", no_init) // no default constructor

      .def(init<std::string, std::function<bool(const Mantid::Kernel::IPropertyManager *, Property *, Property *)>>(
          (arg("self"), arg("watchedPropName"), arg("changeCriterion")),
          "Update dynamic-default value of property when criterion is satisfied for watched property."));
}
