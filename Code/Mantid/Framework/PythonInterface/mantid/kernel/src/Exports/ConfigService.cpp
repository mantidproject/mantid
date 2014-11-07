#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
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

namespace
{
  /// Set directories from a python list
  void setDataSearchDirs(ConfigServiceImpl &self, const boost::python::list & paths)
  {
    using namespace Mantid::PythonInterface;
    self.setDataSearchDirs(Converters::PySequenceToVector<std::string>(paths)());
  }


   /// Forward call from __getitem__ to getString with use_cache_true
   std::string getStringUsingCache(ConfigServiceImpl &self, const std::string & key)
   {
     return self.getString(key, true);
   }

  /// Overload generator for getInstrument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getInstrument_Overload, getInstrument, 0, 1);
  /// Overload generator for getString
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getString_Overload, getString, 1, 2);
}

void export_ConfigService()
{
  using Mantid::PythonInterface::std_vector_exporter;

  std_vector_exporter<FacilityInfo*>::wrap("std_vector_facilityinfo");
  
  class_<ConfigServiceImpl, boost::noncopyable>("ConfigServiceImpl", no_init)
    .def("reset", &ConfigServiceImpl::reset,
         "Clears all user settings and removes the user properties file")

    .def("getLocalFilename", &ConfigServiceImpl::getLocalFilename, "Returns the path to the system wide properties file.")

    .def("getUserFilename", &ConfigServiceImpl::getUserFilename, "Returns the path to the user properties file")

    .def("getInstrumentDirectory", &ConfigServiceImpl::getInstrumentDirectory,
         "Returns the directory used for the instrument definitions")

	.def("getInstrumentDirectories", &ConfigServiceImpl::getInstrumentDirectories, return_value_policy<reference_existing_object>(), 
         "Returns the list of directories searched for the instrument definitions")

    .def("getFacilityNames", &ConfigServiceImpl::getFacilityNames, "Returns the default facility")

    .def("getFacilities", &ConfigServiceImpl::getFacilities, "Returns the default facility")

    .def("getFacility", (const FacilityInfo&(ConfigServiceImpl::*)() const)&ConfigServiceImpl::getFacility,
        return_value_policy<reference_existing_object>(), "Returns the default facility")

    .def("getFacility", (const FacilityInfo&(ConfigServiceImpl::*)(const std::string&) const)&ConfigServiceImpl::getFacility,
         (arg("facilityName")), return_value_policy<reference_existing_object>(),
         "Returns the named facility. Raises an RuntimeError if it does not exist")

    .def("setFacility", &ConfigServiceImpl::setFacility, "Sets the current facility to the given name")

    .def("updateFacilities", &ConfigServiceImpl::updateFacilities, "Loads facility information from a provided file")

    .def("getInstrument", &ConfigServiceImpl::getInstrument,
          getInstrument_Overload("Returns the named instrument. If name = \"\" then the default.instrument is returned",
                                 (arg("instrumentName")=""))[return_value_policy<copy_const_reference>()])

    .def("getString", &ConfigServiceImpl::getString,
          getString_Overload("Returns the named key's value. If use_cache = true [default] then relative paths->absolute",
                             (arg("key"),arg("use_cache")=true)))

    .def("setString", &ConfigServiceImpl::setString, "Set the given property name. "
         "If it does not exist it is added to the current configuration")

    .def("hasProperty", &ConfigServiceImpl::hasProperty)

    .def("getDataSearchDirs",&ConfigServiceImpl::getDataSearchDirs, return_value_policy<copy_const_reference>(),
         "Return the current list of data search paths")

    .def("appendDataSearchDir", &ConfigServiceImpl::appendDataSearchDir,
         "Append a directory to the current list of data search paths")

    .def("setDataSearchDirs", (void (ConfigServiceImpl::*)(const std::string &))&ConfigServiceImpl::setDataSearchDirs,
         "Set the whole datasearch.directories property from a single string. Entries should be separated by a ; character")

    .def("setDataSearchDirs", &setDataSearchDirs,
         "Set the  datasearch.directories property from a list of strings.")

    .def("saveConfig", &ConfigServiceImpl::saveConfig, "Saves the keys that have changed from their default to the given filename")

    // Treat this as a dictionary
    .def("__getitem__", &getStringUsingCache)
    .def("__setitem__", &ConfigServiceImpl::setString)
    .def("__contains__", &ConfigServiceImpl::hasProperty)
    .def("Instance", &ConfigService::Instance,  return_value_policy<reference_existing_object>(),
         "Returns a reference to the ConfigService")
    .staticmethod("Instance")

    ;
}

