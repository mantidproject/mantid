#include "MantidAPI/FileProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include <boost/python/class.hpp>
#include <boost/python/bases.hpp>

using Mantid::API::FileProperty;
using Mantid::Kernel::PropertyWithValue;

using boost::python::class_;
using boost::python::no_init;
using boost::python::bases;

void export_FileProperty()
{
  class_<FileProperty, bases<PropertyWithValue<std::string> >, boost::noncopyable>("FileProperty", no_init)
    ;
}

