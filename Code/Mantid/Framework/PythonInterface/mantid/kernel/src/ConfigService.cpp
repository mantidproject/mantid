#include "MantidKernel/ConfigService.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;
using namespace boost::python;

namespace
{
  /**
   * Getter to return the singleton reference
   * @returns A reference to the ConfigServic singleton
   */
  ConfigServiceImpl & getConfigService()
  {
    return ConfigService::Instance();
  }

}

void export_ConfigService()
{
  class_<ConfigServiceImpl, boost::noncopyable>("ConfigService", no_init)
     .def("get_data_dirs",&ConfigServiceImpl::getDataSearchDirs, return_value_policy<copy_const_reference>(),
         "Return the current list of data search paths")
     // Treat this as a dictionary
     .def("__getitem__", (std::string (ConfigServiceImpl::*)(const std::string &))&ConfigServiceImpl::getString)
     .def("__setitem__", &ConfigServiceImpl::setString)
     .def("__contains__", &ConfigServiceImpl::hasProperty)
          ;

  def("get_config_service", &getConfigService,  return_value_policy<reference_existing_object>(),
      "Returns a reference to the ConfigService");

}

