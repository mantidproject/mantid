// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::Run;
using Mantid::Geometry::Goniometer;
using Mantid::Kernel::Property;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Goniometer)
GET_POINTER_SPECIALIZATION(Run)

namespace {
namespace bpl = boost::python;

/**
 * Add a property with the given name and value
 * @param self A reference to the run object that we have been called on
 * @param name The name of the new property
 * @param value The value of the property
 * @param units A string representing a unit
 * @param replace If true, replace an existing property with this one else
 * raise an error
 */
void addPropertyWithUnit(Run &self, const std::string &name,
                         const bpl::object &value, const std::string &units,
                         bool replace) {
  extract<Property *> extractor(value);
  if (extractor.check()) {
    Property *prop = extractor();
    self.addProperty(
        prop->clone(),
        replace); // Clone the property as Python owns the one that is passed in
    return;
  }

  // Use the factory
  auto property =
      Mantid::PythonInterface::Registry::PropertyWithValueFactory::create(
          name, value, Mantid::Kernel::Direction::Input);
  property->setUnits(units);
  self.addProperty(std::move(property), replace);
}

/**
 * Add a property with the given name and value
 * @param self A reference to the run object that we have been called on
 * @param name The name of the new property
 * @param value The value of the property
 * @param replace If true, replace an existing property with this one else raise
 * an error
 */
void addProperty(Run &self, const std::string &name, const bpl::object &value,
                 bool replace) {
  addPropertyWithUnit(self, name, value, "", replace);
}

/**
 * Add a property with the given name and value. An existing property is
 * overwritten if it exists
 * @param self A reference to the run object that we have been called on
 * @param name The name of the new property
 * @param value The value of the property
 */
void addOrReplaceProperty(Run &self, const std::string &name,
                          const bpl::object &value) {
  addProperty(self, name, value, true);
}

/**
 * Emulate dict.get. Returns the value pointed to by the key or the default
 * given
 * @param self The object called on
 * @param key The key
 * @param default_ The default to return if it does not exist
 */
bpl::object getWithDefault(bpl::object self, bpl::object key,
                           bpl::object default_) {
  bpl::object exists(self.attr("__contains__"));
  if (extract<bool>(exists(key))()) {
    return self.attr("__getitem__")(key);
  } else {
    return default_;
  }
}

/**
 * Emulate dict.get. Returns the value pointed to by the key or None if it
 * doesn't exist
 * @param self The bpl::object called on
 * @param key The key
 */
bpl::object get(bpl::object self, bpl::object key) {
  return getWithDefault(self, key, bpl::object());
}

/**
 * Emulate dict.keys. Returns a list of property names know to the run object
 * @param self :: A reference to the Run object that called this method
 */
bpl::list keys(Run &self) {
  const std::vector<Mantid::Kernel::Property *> &logs = self.getProperties();
  bpl::list names;
  for (auto log : logs) {
    names.append(log->name());
  }
  return names;
}
} // namespace
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(integrateProtonCharge_Overload,
                                       integrateProtonCharge, 0, 1)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

void export_Run() {
  // Pointer
  register_ptr_to_python<Run *>();

  // Run class
  class_<Run, boost::noncopyable>("Run", no_init)
      .def("getProtonCharge", &Run::getProtonCharge, arg("self"),
           "Return the total good proton charge for the run")

      .def("integrateProtonCharge", &Run::integrateProtonCharge,
           integrateProtonCharge_Overload(
               "Set the total good proton charge for the run, from the proton "
               "charge log",
               (arg("self"), arg("logname") = "proton_charge")))

      .def("hasProperty", &Run::hasProperty, (arg("self"), arg("name")),
           "Returns True if the given log value is contained within the run")

      .def("getProperty", &Run::getProperty, (arg("self"), arg("name")),
           return_value_policy<return_by_value>(),
           "Returns the named property "
           "(log value). Use '.value' "
           "to return the value.")

      .def("getProperties", &Run::getProperties, arg("self"),
           return_internal_reference<>(),
           "Return the list of run properties managed by this object.")

      .def("getLogData",
           (Property * (Run::*)(const std::string &)const) & Run::getLogData,
           (arg("self"), arg("name")), return_value_policy<return_by_value>(),
           "Returns the named log. Use '.value' to return the value. The same "
           "as getProperty.")

      .def("getLogData",
           (const std::vector<Property *> &(Run::*)() const) & Run::getLogData,
           arg("self"), return_internal_reference<>(),
           "Return the list of logs for this run. The same as getProperties.")

      .def("getGoniometer",
           (const Mantid::Geometry::Goniometer &(Run::*)() const) &
               Run::getGoniometer,
           arg("self"), return_value_policy<reference_existing_object>(),
           "Return the Goniometer object associated with this run.")

      .def("addProperty", &addProperty,
           (arg("self"), arg("name"), arg("value"), arg("replace")),
           "Adds a property with the given name "
           "and value. If replace=True then an "
           "existing property is overwritten")

      .def("addProperty", &addPropertyWithUnit,
           (arg("self"), arg("name"), arg("value"), arg("units"),
            arg("replace")),
           "Adds a property with the given name, value and unit. If "
           "replace=True then an existing property is overwritten")

      .def("setStartAndEndTime", &Run::setStartAndEndTime,
           (arg("self"), arg("start"), arg("end")),
           "Set the start and end time of the run")

      .def("startTime", &Run::startTime, arg("self"),
           "Return the total starting time of the run.")

      .def("endTime", &Run::endTime, arg("self"),
           "Return the total ending time of the run.")

      //--------------------------- Dictionary
      // access----------------------------
      .def("get", &getWithDefault, (arg("self"), arg("key"), arg("default")),
           "Returns the value pointed to by the key or the default value "
           "given.")
      .def("get", &get, (arg("self"), arg("key")),
           "Returns the value pointed to by the key or "
           "None if it does not exist.")
      .def("keys", &keys, arg("self"),
           "Returns the names of the properties as list")
      .def("__contains__", &Run::hasProperty, (arg("self"), arg("name")))
      .def("__getitem__", &Run::getProperty, (arg("self"), arg("name")),
           return_value_policy<return_by_value>())
      .def("__setitem__", &addOrReplaceProperty,
           (arg("self"), arg("name"), arg("value")))

      ;
}
