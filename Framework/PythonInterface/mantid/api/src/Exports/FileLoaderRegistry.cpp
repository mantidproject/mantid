#include "MantidAPI/FileLoaderRegistry.h"
#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::FileLoaderRegistry;
using Mantid::API::FileLoaderRegistryImpl;
using namespace boost::python;

void export_FileLoaderRegistry() {
  class_<FileLoaderRegistryImpl, boost::noncopyable>("FileLoaderRegistryImpl",
                                                     no_init)
      .def("canLoad", &FileLoaderRegistryImpl::canLoad,
           "Perform a check that that the given algorithm can load the file")
      .def("Instance", &FileLoaderRegistry::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the FileLoaderRegistry singleton instance")
      .staticmethod("Instance");
}