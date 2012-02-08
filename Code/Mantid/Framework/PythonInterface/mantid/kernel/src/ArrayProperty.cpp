#include "MantidKernel/ArrayProperty.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::PropertyWithValue;
using namespace boost::python;

#define EXPORT_ARRAY_PROP(type, suffix) \
    class_<ArrayProperty<type>, \
           bases<PropertyWithValue<std::vector<type> > >, boost::noncopyable>("ArrayProperty_"#suffix, no_init);


void export_ArrayProperty()
{
  EXPORT_ARRAY_PROP(double,dbl);
  EXPORT_ARRAY_PROP(int,int);
  EXPORT_ARRAY_PROP(size_t,size_t);
  EXPORT_ARRAY_PROP(std::string, std_string);
}

