#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/StlExportDefinitions.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;
using Mantid::Kernel::FacilityInfo;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ConfigServiceImpl)

namespace {
/// Set directories from a python list
void setDataSearchDirs(ConfigServiceImpl &self,
                       const boost::python::list &paths) {
  using namespace Mantid::PythonInterface;
  self.setDataSearchDirs(Converters::PySequenceToVector<std::string>(paths)());
}

/// Forward call from __getitem__ to getString with use_cache_true
std::string getStringUsingCache(ConfigServiceImpl &self,
                                const std::string &key) {
  return self.getString(key, true);
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GCC_DIAG_OFF(conversion)
/// Overload generator for getInstrument
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getInstrument_Overload, getInstrument, 0,
                                       1)
/// Overload generator for getString
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getString_Overload, getString, 1, 2)
GCC_DIAG_ON(conversion)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

void export_ConfigService() {
  using Mantid::PythonInterface::std_vector_exporter;

  std_vector_exporter<FacilityInfo *>::wrap("std_vector_facilityinfo");

  class_<ConfigServiceImpl, boost::noncopyable>("ConfigServiceImpl", no_init)
      .def("reset", &ConfigServiceImpl::reset, arg("self"),
           "Clears all user settings and removes the user properties file")

      .def("getAppDataDirectory", &ConfigServiceImpl::getAppDataDir,
           arg("self"), "Returns the path to Mantid's application directory")

      .def("getLocalFilename", &ConfigServiceImpl::getLocalFilename,
           arg("self"), "Returns the path to the system wide properties file.")

      .def("getUserFilename", &ConfigServiceImpl::getUserFilename, arg("self"),
           "Returns the path to the user properties file")

      .def("getUserPropertiesDir", &ConfigServiceImpl::getUserPropertiesDir,
           arg("self"),
           "Returns the directory to use to write out Mantid information")

      .def("getInstrumentDirectory", &ConfigServiceImpl::getInstrumentDirectory,
           arg("self"),
           "Returns the directory used for the instrument definitions")

      .def("getInstrumentDirectories",
           &ConfigServiceImpl::getInstrumentDirectories, arg("self"),
           return_value_policy<reference_existing_object>(),
           "Returns the list of directories searched for the instrument "
           "definitions")

      .def("getFacilityNames", &ConfigServiceImpl::getFacilityNames,
           arg("self"), "Returns the default facility")

      .def("getFacilities", &ConfigServiceImpl::getFacilities, arg("self"),
           "Returns the default facility")

      .def("getFacility", (const FacilityInfo &(ConfigServiceImpl::*)() const) &
                              ConfigServiceImpl::getFacility,
           arg("self"), return_value_policy<reference_existing_object>(),
           "Returns the default facility")

      .def("getFacility",
           (const FacilityInfo &(ConfigServiceImpl::*)(const std::string &)
                const) &
               ConfigServiceImpl::getFacility,
           (arg("self"), arg("facilityName")),
           return_value_policy<reference_existing_object>(),
           "Returns the named facility. Raises an RuntimeError if it does not "
           "exist")

      .def("setFacility", &ConfigServiceImpl::setFacility,
           (arg("self"), arg("facilityName")),
           "Sets the current facility to the given name")

      .def("updateFacilities", &ConfigServiceImpl::updateFacilities,
           (arg("self"), arg("fileName")),
           "Loads facility information from a provided file")

      .def("getInstrument", &ConfigServiceImpl::getInstrument,
           getInstrument_Overload(
               "Returns the named instrument. If name = \"\" then the "
               "default.instrument is returned",
               (arg("self"),
                arg("instrumentName") =
                    ""))[return_value_policy<copy_const_reference>()])

      .def("getString", &ConfigServiceImpl::getString,
           getString_Overload(
               "Returns the named key's value. If use_cache = "
               "true [default] then relative paths->absolute",
               (arg("self"), arg("key"), arg("use_cache") = true)))

      .def("setString", &ConfigServiceImpl::setString,
           (arg("self"), arg("key"), arg("value")),
           "Set the given property name. "
           "If it does not exist it is added to the current configuration")

      .def("hasProperty", &ConfigServiceImpl::hasProperty,
           (arg("self"), arg("rootName")))

      .def("getDataSearchDirs", &ConfigServiceImpl::getDataSearchDirs,
           arg("self"), return_value_policy<copy_const_reference>(),
           "Return the current list of data search paths")

      .def("appendDataSearchDir", &ConfigServiceImpl::appendDataSearchDir,
           (arg("self"), arg("path")),
           "Append a directory to the current list of data search paths")

      .def("appendDataSearchSubDir", &ConfigServiceImpl::appendDataSearchSubDir,
           (arg("self"), arg("subdir")),
           "Appends a sub-directory to each data search directory "
           "and appends the new paths back to datasearch directories")

      .def("setDataSearchDirs",
           (void (ConfigServiceImpl::*)(const std::string &)) &
               ConfigServiceImpl::setDataSearchDirs,
           (arg("self"), arg("searchDirs")),
           "Set the whole datasearch.directories property from a single "
           "string. Entries should be separated by a ; character")

      .def("setDataSearchDirs", &setDataSearchDirs,
           (arg("self"), arg("searchDirs")),
           "Set the  datasearch.directories property from a list of strings.")

      .def("saveConfig", &ConfigServiceImpl::saveConfig,
           (arg("self"), arg("filename")),
           "Saves the keys that have changed from their default to the given "
           "filename")

      .def("setConsoleLogLevel", &ConfigServiceImpl::setConsoleLogLevel,
           (arg("self"), arg("logLevel")),
           "Sets the log level priority for the Console log channel, logLevel "
           "1 = Fatal, 6 = information, 7 = Debug")

      .def("keys", &ConfigServiceImpl::keys, arg("self"))

      // Treat this as a dictionary
      .def("__getitem__", &getStringUsingCache, (arg("self"), arg("key")))
      .def("__setitem__", &ConfigServiceImpl::setString,
           (arg("self"), arg("key"), arg("value")))
      .def("__contains__", &ConfigServiceImpl::hasProperty,
           (arg("self"), arg("key")))
      .def("Instance", &ConfigService::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the ConfigService")
      .staticmethod("Instance")

      ;
}
