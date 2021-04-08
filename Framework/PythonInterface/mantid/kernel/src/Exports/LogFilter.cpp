// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/python/class.hpp>
#include <boost/python/return_by_value.hpp>

using Mantid::Kernel::LogFilter;
using Mantid::Kernel::Property;

using namespace boost::python;

void export_LogFilter() {
  class_<LogFilter, boost::noncopyable>(
      "LogFilter",
      init<const Property *>((arg("self"), arg("property")), "Creates a log filter using the log to be filtered"))
      .def("data", &LogFilter::data, arg("self"), return_value_policy<return_by_value>(),
           "Returns a time series property filtered on current filter property")

      .def("addFilter", &LogFilter::addFilter, (arg("self"), arg("filter")), "Adds a filter to the current list");
}
