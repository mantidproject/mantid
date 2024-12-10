// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"
#include "MantidPythonInterface/core/StlExportDefinitions.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;
using Mantid::Kernel::FacilityInfo;
using Mantid::Kernel::InstrumentInfo;
using Mantid::PythonInterface::ReleaseGlobalInterpreterLock;
using Mantid::PythonInterface::Converters::PySequenceToVector;

using namespace boost::python;

using ExtractStdString = boost::python::extract<std::string>;

GET_POINTER_SPECIALIZATION(ConfigServiceImpl)

namespace {
/// Set directories from a python list
void setDataSearchDirs(ConfigServiceImpl &self, const object &paths) {
  ExtractStdString singleString(paths);
  if (singleString.check()) {
    self.setDataSearchDirs(singleString());
  } else {
    self.setDataSearchDirs(PySequenceToVector<std::string>(paths)());
  }
}

/// Forward call from __getitem__ to getString with use_cache_true
std::string getStringUsingCache(ConfigServiceImpl const *const self, const std::string &key) {
  ReleaseGlobalInterpreterLock releaseGIL;
  return self->getString(key, true);
}

const InstrumentInfo &getInstrument(ConfigServiceImpl const *const self, const object &name = object()) {
  if (name.is_none())
    return self->getInstrument();
  else
    return self->getInstrument(ExtractStdString(name)());
}

/// duck typing emulating dict.get method
std::string getStringUsingCacheElseDefault(ConfigServiceImpl const *const self, const std::string &key,
                                           const std::string &defaultValue) {
  ReleaseGlobalInterpreterLock releaseGIL;
  if (self->hasProperty(key))
    return self->getString(key, true);
  else
    return defaultValue;
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Overload generator for getInstrument
BOOST_PYTHON_FUNCTION_OVERLOADS(getInstrument_Overload, getInstrument, 1, 2)
// Overload generator for getString
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getString_Overload, ConfigServiceImpl::getString, 1, 2)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace

void export_ConfigService() {
  using Mantid::PythonInterface::std_vector_exporter;

  std_vector_exporter<FacilityInfo *>::wrap("std_vector_facilityinfo");

  class_<ConfigServiceImpl, boost::noncopyable>("ConfigServiceImpl", no_init)
      .def("reset", &ConfigServiceImpl::reset, arg("self"),
           "Clears all user settings and removes the user properties file")
      .def("getAppDataDirectory", &ConfigServiceImpl::getAppDataDir, arg("self"),
           "Returns the path to Mantid's application directory")
      .def("getLocalFilename", &ConfigServiceImpl::getLocalFilename, arg("self"),
           "Returns the path to the system wide properties file.")
      .def("getUserFilename", &ConfigServiceImpl::getUserFilename, arg("self"),
           "Returns the path to the user properties file")
      .def("getPropertiesDir", &ConfigServiceImpl::getPropertiesDir, arg("self"),
           "Returns the directory containing the Mantid.properties file.")
      .def("getUserPropertiesDir", &ConfigServiceImpl::getUserPropertiesDir, arg("self"),
           "Returns the directory to use to write out Mantid information")
      .def("getInstrumentDirectory", &ConfigServiceImpl::getInstrumentDirectory, arg("self"),
           "Returns the directory used for the instrument definitions")
      .def("getInstrumentDirectories", &ConfigServiceImpl::getInstrumentDirectories, arg("self"),
           return_value_policy<reference_existing_object>(),
           "Returns the list of directories searched for the instrument "
           "definitions")
      .def("getFacilityNames", &ConfigServiceImpl::getFacilityNames, arg("self"), "Returns the default facility")
      .def("getFacilities", &ConfigServiceImpl::getFacilities, arg("self"), "Returns the default facility")
      .def("configureLogging", &ConfigServiceImpl::configureLogging, arg("self"),
           "Configure and start the logging framework")
      .def("remove", &ConfigServiceImpl::remove, (arg("self"), arg("rootName")), "Remove the indicated key.")
      .def("getFacility", (const FacilityInfo &(ConfigServiceImpl::*)() const) & ConfigServiceImpl::getFacility,
           arg("self"), return_value_policy<reference_existing_object>(), "Returns the default facility")
      .def("getFacility",
           (const FacilityInfo &(ConfigServiceImpl::*)(const std::string &) const) & ConfigServiceImpl::getFacility,
           (arg("self"), arg("facilityName")), return_value_policy<reference_existing_object>(),
           "Returns the named facility. Raises an RuntimeError if it does not "
           "exist")
      .def("setFacility", &ConfigServiceImpl::setFacility, (arg("self"), arg("facilityName")),
           "Sets the current facility to the given name")
      .def("updateFacilities", &ConfigServiceImpl::updateFacilities, (arg("self"), arg("fileName")),
           "Loads facility information from a provided file")
      .def("getInstrument", &getInstrument,
           getInstrument_Overload(
               "Returns the named instrument. If name = \"\" then the "
               "default.instrument is returned",
               (arg("self"),
                arg("instrumentName") = boost::python::object()))[return_value_policy<copy_const_reference>()])
      .def("getString", &ConfigServiceImpl::getString,
           getString_Overload("Returns the named key's value. If use_cache = "
                              "true [default] then relative paths->absolute",
                              (arg("self"), arg("key"), arg("pathAbsolute") = true)))

      .def("setString", &ConfigServiceImpl::setString, (arg("self"), arg("key"), arg("value")),
           "Set the given property name. "
           "If it does not exist it is added to the current configuration")
      .def("hasProperty", &ConfigServiceImpl::hasProperty, (arg("self"), arg("rootName")))
      .def("getDataSearchDirs", &ConfigServiceImpl::getDataSearchDirs, arg("self"),
           return_value_policy<copy_const_reference>(), "Return the current list of data search paths")
      .def("appendDataSearchDir", &ConfigServiceImpl::appendDataSearchDir, (arg("self"), arg("path")),
           "Append a directory to the current list of data search paths")
      .def("appendDataSearchSubDir", &ConfigServiceImpl::appendDataSearchSubDir, (arg("self"), arg("subdir")),
           "Appends a sub-directory to each data search directory "
           "and appends the new paths back to datasearch directories")
      .def("setDataSearchDirs", &setDataSearchDirs, (arg("self"), arg("searchDirs")),
           "Set the datasearch.directories property from a list of strings or "
           "a single ';' separated string.")
      .def("saveConfig", &ConfigServiceImpl::saveConfig, (arg("self"), arg("filename")),
           "Saves the keys that have changed from their default to the given "
           "filename")
      .def("getLogLevel", &ConfigServiceImpl::getLogLevel, arg("self"),
           "Return the string value for the log representation")
      .def("setLogLevel", (void(ConfigServiceImpl::*)(int, bool)) & ConfigServiceImpl::setLogLevel,
           (arg("self"), arg("logLevel"), arg("quiet") = false),
           "Sets the log level priority for all the log channels, logLevel 1 = Fatal, 6 = information, 7 = Debug")
      .def("setLogLevel", (void(ConfigServiceImpl::*)(std::string const &, bool)) & ConfigServiceImpl::setLogLevel,
           (arg("self"), arg("logLevel"), arg("quiet") = false),
           "Sets the log level priority for all the log channels. Allowed values are fatal, critical, error, warning, "
           "notice, information, debug, and trace.")
      .def("keys", &ConfigServiceImpl::keys, arg("self"))

      // Treat this as a dictionary
      .def("get", &getStringUsingCache, (arg("self"), arg("key")),
           "get the string value of a property; return empty string value if the property "
           "is not found in the configuration")
      .def("get", &getStringUsingCacheElseDefault, (arg("self"), arg("key")), arg("default"),
           "get the string value of a property; return a default string value if the property "
           "is not found in the configuration")
      .def("__getitem__", &getStringUsingCache, (arg("self"), arg("key")))
      .def("__setitem__", &ConfigServiceImpl::setString, (arg("self"), arg("key"), arg("value")))
      .def("__contains__", &ConfigServiceImpl::hasProperty, (arg("self"), arg("key")))
      .def("Instance", &ConfigService::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the ConfigService")
      .staticmethod("Instance");
}
