#include "MantidKernel/LogFilter.h"
#include <boost/python/class.hpp>
#include <boost/python/return_by_value.hpp>

using Mantid::Kernel::LogFilter;
using Mantid::Kernel::Property;

using namespace boost::python;

void export_LogFilter()
{
  class_<LogFilter,boost::noncopyable>("LogFilter", 
                                        init<const Property*>("Creates a log filter using the log to be filtered"))
    .def("data", &LogFilter::data, return_value_policy<return_by_value>(), 
         "Returns a time series property filtered on current filter property")

    .def("addFilter", &LogFilter::addFilter, "Adds a filter to the current list")
    ;
}

