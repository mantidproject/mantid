// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Property.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/PythonObjectInstantiator.h"
#include "MantidPythonInterface/core/StlExportDefinitions.h"
#include <array>

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::Direction;
using Mantid::Kernel::Property;
using Mantid::PythonInterface::std_vector_exporter;
using namespace boost::python;

namespace {

//
// gcc 7 with std=c++17 has an issue attaching the EMPTY_*
// functions with add_static_property when attempting to cast
// the function pointer to a "const volatile void *":
//
//   arg_to_python.hpp:211:66: error: invalid conversion from 'double (*)()
//   noexcept'
//                             to 'const volatile void*' [-fpermissive]
//
// The noexcept specification appears to prevent the cast in the
// boost python layer. These functions provide a pass through without the
// noexcept specifier.

constexpr inline double emptyDouble() { return Mantid::EMPTY_DBL(); }

constexpr inline int emptyInt() { return Mantid::EMPTY_INT(); }

constexpr inline long emptyLong() { return Mantid::EMPTY_LONG(); }

/**
 * Return the units string as a Python unicode object. Tries encoding as utf-8
 * first followed by a list of fallback encodings to catch things like windows
 * encodings in old ISIS files
 */
PyObject *unitAsUnicode(const Property &self) {
  static constexpr std::array<const char *, 2> codecs{"utf-8", "windows-1252"};
  const auto &unitsBytes = self.units();
  for (const auto &encoding : codecs) {
    auto unitsPy = PyUnicode_Decode(unitsBytes.c_str(), unitsBytes.size(), encoding, "strict");
    if (unitsPy)
      return unitsPy;
    // encoding failed. Try next
    PyErr_Clear();
  }
  // All attempts failed, create an appropriate error string
  const std::string allCodecsStr = []() {
    auto it = codecs.begin();
    std::string result = *it++;
    for (; it != codecs.end(); ++it) {
      result.append(",");
      result.append(*it);
    }
    return result;
  }();
  const std::string helpMessage = std::string("Can't decode units string. Tried codecs=")
                                      .append(allCodecsStr)
                                      .append("\nTo try other codecs use Property.unitsAsBytes to retrieve the "
                                              "original bytes object and use .decode().");
  PyErr_SetString(PyExc_RuntimeError, helpMessage.c_str());
  throw error_already_set();
}

/**
 * Return the units string as a Python bytes object. Provides access
 * to the raw bytes in case the standar .units conversion fails with an decoding
 * error
 */
PyObject *unitsAsBytes(const Property &self) { return PyBytes_FromString(self.units().c_str()); }

} // namespace

GET_POINTER_SPECIALIZATION(Property)
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(valueAsPrettyStrOverloader,

                                       valueAsPrettyStr, 0, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

void export_Property() {
  register_ptr_to_python<Property *>();

  // vector of properties
  std_vector_exporter<Property *>::wrap("std_vector_property");

  // Direction
  enum_<Direction::Type>("Direction")
      .value("Input", Direction::Input)
      .value("Output", Direction::Output)
      .value("InOut", Direction::InOut)
      .value("None", Direction::None);

  // Add properties as that's what old version had
  class_<Property, boost::noncopyable>("Property", no_init)
      .add_property("name", make_function(&Property::name, return_value_policy<copy_const_reference>()),
                    "The name of the property")

      .add_property("isValid", make_function(&Property::isValid),
                    "An empty string if the property is valid, otherwise it "
                    "contains an error message.")

      .add_property("isDefault", make_function(&Property::isDefault), "Is the property set at the default value")

      .add_property("getDefault", make_function(&Property::getDefault), "Get the default value as a string")

      .add_property("direction", &Property::direction, "Input, Output, InOut or Unknown. See the Direction class")

      .add_property("documentation",
                    make_function(&Property::documentation, return_value_policy<copy_const_reference>()),
                    "The property's doc string")

      .def("setDocumentation", &Property::setDocumentation, (arg("doc")), "Setting documentation for property")

      .add_property("type", make_function(&Property::type), "Returns a string identifier for the type")

      .add_property("units", &unitAsUnicode, &Property::setUnits, "The units attached to this property")

      .add_property("unitsAsBytes", &unitsAsBytes,
                    "The units attached to this property as a encoded bytes object. It "
                    "is assumed the caller knows the correct endcoding used.")

      .add_property("valueAsStr", &Property::value, &Property::setValue,
                    "The value of the property as a string. "
                    "For some property types, e.g. Workspaces, it is useful to "
                    "be able to refer to the string value directly")

      .def("valueAsPrettyStr", &Property::valueAsPrettyStr,
           valueAsPrettyStrOverloader((arg("maxLength") = 0, arg("collapseLists") = true),
                                      "The value of the property as a formatted string. "
                                      "If maxLength is defined then the output may not contain the "
                                      "full "
                                      "contents of the property. The maxLength and collapseLists "
                                      "arguments "
                                      "do not work for all property types"))

      .add_property("allowedValues", &Property::allowedValues, "A list of allowed values")

      .add_property("getGroup", make_function(&Property::getGroup, return_value_policy<copy_const_reference>()),
                    "Return the 'group' of the property, that is, the header "
                    "in the algorithm's list of properties.")

      .add_property("settings", make_function(&Property::getSettings, return_value_policy<return_by_value>()),
                    "Return the object managing this property's settings")

      .add_static_property("EMPTY_DBL", emptyDouble)
      .add_static_property("EMPTY_INT", emptyInt)
      .add_static_property("EMPTY_LONG", emptyLong)

      .def("setAutoTrim", &Property::setAutoTrim, (arg("setting")), "Setting automatic trimming of whitespaces.")
      .def("getAutoTrim", &Property::autoTrim, "Gets the setting of automatic trimming of whitespaces.")
      .def("setDisableReplaceWSButton", &Property::setDisableReplaceWSButton, (arg("disable")),
           "Disable the creation of the Replace Workspace button.");
}
