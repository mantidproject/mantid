#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
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
    self.setDataSearchDirs(Converters::PySequenceToVectorConverter<std::string>(paths)());
  }

  /// Overload generator for getInstrument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getInstrument_Overload, getInstrument, 0, 1);
}

void export_ConfigService()
{
  class_<ConfigServiceImpl, boost::noncopyable>("ConfigServiceImpl", no_init)
    .def("getLocalFilename", &ConfigServiceImpl::getLocalFilename, "Returns the path to the system wide properties file.")

    .def("getUserFilename", &ConfigServiceImpl::getUserFilename, "Returns the path to the user properties file")

    .def("getInstrumentDirectory", &ConfigServiceImpl::getInstrumentDirectory,
         "Returns the directory used for the instrument definitions")

    .def("getFacility", (const FacilityInfo&(ConfigServiceImpl::*)() const)&ConfigServiceImpl::getFacility,
        return_value_policy<reference_existing_object>(), "Returns the default facility")

    .def("getFacility", (const FacilityInfo&(ConfigServiceImpl::*)(const std::string&) const)&ConfigServiceImpl::getFacility,
         (arg("facilityName")), return_value_policy<reference_existing_object>(),
         "Returns the named facility. Raises an RuntimeError if it does not exist")

    .def("setFacility", &ConfigServiceImpl::setString, (arg("facilityName")), "Sets the current facility to the given name")

    .def("getInstrument", &ConfigServiceImpl::getInstrument,
          getInstrument_Overload("Returns the named instrument. If name = \"\" then the default.instrument is returned",
                                 (arg("instrumentName")=""))[return_value_policy<copy_const_reference>()])

    .def("getString", (std::string (ConfigServiceImpl::*)(const std::string &))&ConfigServiceImpl::getString,
         "Return the given property")

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

    // Treat this as a dictionary
    .def("__getitem__", (std::string (ConfigServiceImpl::*)(const std::string &))&ConfigServiceImpl::getString)
    .def("__setitem__", &ConfigServiceImpl::setString)
    .def("__contains__", &ConfigServiceImpl::hasProperty)
    .def("Instance", &ConfigService::Instance,  return_value_policy<reference_existing_object>(),
         "Returns a reference to the ConfigService")
    .staticmethod("Instance")

    ;
}

