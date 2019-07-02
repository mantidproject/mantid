// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::FilteredTimeSeriesProperty;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::PythonInterface::Policies::RemoveConst;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(FilteredTimeSeriesProperty<std::string>)
GET_POINTER_SPECIALIZATION(FilteredTimeSeriesProperty<int32_t>)
GET_POINTER_SPECIALIZATION(FilteredTimeSeriesProperty<int64_t>)
GET_POINTER_SPECIALIZATION(FilteredTimeSeriesProperty<bool>)
GET_POINTER_SPECIALIZATION(FilteredTimeSeriesProperty<double>)

namespace {
/// Macro to reduce copy-and-paste
#define EXPORT_FILTEREDTIMESERIES_PROP(TYPE, Prefix)                           \
  register_ptr_to_python<FilteredTimeSeriesProperty<TYPE> *>();                \
                                                                               \
  class_<FilteredTimeSeriesProperty<TYPE>, bases<TimeSeriesProperty<TYPE>>,    \
         boost::noncopyable>(#Prefix "FilteredTimeSeriesProperty", no_init)    \
      .def(init<TimeSeriesProperty<TYPE> *, const TimeSeriesProperty<bool> &>( \
          "Constructor", (arg("self"), arg("source"), arg("filter"))))         \
      .def("unfiltered", &FilteredTimeSeriesProperty<TYPE>::unfiltered,        \
           (arg("self")), return_value_policy<RemoveConst>(),                  \
           "Returns a time series containing the unfiltered data");
}

void export_FilteredTimeSeriesProperty() {
  EXPORT_FILTEREDTIMESERIES_PROP(double, Float);
  EXPORT_FILTEREDTIMESERIES_PROP(bool, Bool);
  EXPORT_FILTEREDTIMESERIES_PROP(int32_t, Int32);
  EXPORT_FILTEREDTIMESERIES_PROP(int64_t, Int64);
  EXPORT_FILTEREDTIMESERIES_PROP(std::string, String);
}
