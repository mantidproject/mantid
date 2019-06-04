// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidPythonInterface/core/Converters/DateAndTime.h"
#include "MantidPythonInterface/kernel/Converters/ContainerDtype.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/init.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/tuple.hpp>

using Mantid::Kernel::Property;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Types::Core::DateAndTime;
using namespace boost::python;
using boost::python::arg;

GET_POINTER_SPECIALIZATION(TimeSeriesProperty<std::string>)
GET_POINTER_SPECIALIZATION(TimeSeriesProperty<int32_t>)
GET_POINTER_SPECIALIZATION(TimeSeriesProperty<int64_t>)
GET_POINTER_SPECIALIZATION(TimeSeriesProperty<bool>)
GET_POINTER_SPECIALIZATION(TimeSeriesProperty<double>)

namespace {

using Mantid::PythonInterface::Policies::VectorToNumpy;

template <typename TYPE>
void addPyTimeValue(TimeSeriesProperty<TYPE> &self,
                    const boost::python::api::object &datetime,
                    const TYPE &value) {
  const auto dateandtime =
      Mantid::PythonInterface::Converters::to_dateandtime(datetime);
  self.addValue(*dateandtime, value);
}

// Call the dtype helper function
template <typename TYPE> std::string dtype(TimeSeriesProperty<TYPE> &self) {
  return Mantid::PythonInterface::Converters::dtype(self);
}

// Check for the special case of a string
template <> std::string dtype(TimeSeriesProperty<std::string> &self) {
  // Vector of ints to store the sizes of each of the strings
  std::vector<size_t> stringSizes;

  // Block allocate memory
  stringSizes.reserve(self.size());

  // Loop for the number of strings in self
  for (int i = 0; i < self.size(); i++) {
    // For each string store the number of characters
    std::string val = self.nthValue(i);
    size_t size = val.size();
    stringSizes.emplace_back(size);
  }

  // Find the maximum number of characters
  size_t max =
      *std::max_element(std::begin(stringSizes), std::end(stringSizes));

  // Create the string to return
  std::stringstream ss;
  ss << "S" << max;
  std::string retVal = ss.str();
  return retVal;
}

// Macro to reduce copy-and-paste
#define EXPORT_TIMESERIES_PROP(TYPE, Prefix)                                   \
  register_ptr_to_python<TimeSeriesProperty<TYPE> *>();                        \
                                                                               \
  class_<TimeSeriesProperty<TYPE>, bases<Property>, boost::noncopyable>(       \
      #Prefix "TimeSeriesProperty",                                            \
      init<const std::string &>((arg("self"), arg("value"))))                  \
      .add_property(                                                           \
          "value",                                                             \
          make_function(                                                       \
              &Mantid::Kernel::TimeSeriesProperty<TYPE>::valuesAsVector,       \
              return_value_policy<VectorToNumpy>()))                           \
      .add_property(                                                           \
          "times",                                                             \
          make_function(                                                       \
              &Mantid::Kernel::TimeSeriesProperty<TYPE>::timesAsVector,        \
              return_value_policy<VectorToNumpy>()))                           \
      .def("addValue",                                                         \
           (void (TimeSeriesProperty<TYPE>::*)(const DateAndTime &,            \
                                               const TYPE)) &                  \
               TimeSeriesProperty<TYPE>::addValue,                             \
           (arg("self"), arg("time"), arg("value")))                           \
      .def("addValue",                                                         \
           (void (TimeSeriesProperty<TYPE>::*)(const std::string &,            \
                                               const TYPE)) &                  \
               TimeSeriesProperty<TYPE>::addValue,                             \
           (arg("self"), arg("time"), arg("value")))                           \
      .def("addValue", &addPyTimeValue<TYPE>,                                  \
           (arg("self"), arg("time"), arg("value")))                           \
      .def("clear", &TimeSeriesProperty<TYPE>::clear, arg("self"))             \
      .def("valueAsString", &TimeSeriesProperty<TYPE>::value, arg("self"))     \
      .def("size", &TimeSeriesProperty<TYPE>::size, arg("self"))               \
      .def("firstTime", &TimeSeriesProperty<TYPE>::firstTime, arg("self"),     \
           "returns :class:`mantid.kernel.DateAndTime`")                       \
      .def("firstValue", &TimeSeriesProperty<TYPE>::firstValue, arg("self"))   \
      .def("lastTime", &TimeSeriesProperty<TYPE>::lastTime, arg("self"),       \
           "returns :class:`mantid.kernel.DateAndTime`")                       \
      .def("lastValue", &TimeSeriesProperty<TYPE>::lastValue, arg("self"))     \
      .def("nthValue", &TimeSeriesProperty<TYPE>::nthValue,                    \
           (arg("self"), arg("index")))                                        \
      .def("nthTime", &TimeSeriesProperty<TYPE>::nthTime,                      \
           (arg("self"), arg("index")),                                        \
           "returns :class:`mantid.kernel.DateAndTime`")                       \
      .def("getStatistics", &TimeSeriesProperty<TYPE>::getStatistics,          \
           arg("self"),                                                        \
           "returns :class:`mantid.kernel.TimeSeriesPropertyStatistics`")      \
      .def("timeAverageValue", &TimeSeriesProperty<TYPE>::timeAverageValue,    \
           arg("self"))                                                        \
      .def("dtype", &dtype<TYPE>, arg("self"));

} // namespace

void export_TimeSeriesProperty_Double() {
  EXPORT_TIMESERIES_PROP(double, Float);
}

void export_TimeSeriesProperty_Bool() { EXPORT_TIMESERIES_PROP(bool, Bool); }

void export_TimeSeriesProperty_Int32() {
  EXPORT_TIMESERIES_PROP(int32_t, Int32);
}

void export_TimeSeriesProperty_Int64() {
  EXPORT_TIMESERIES_PROP(int64_t, Int64);
}

void export_TimeSeriesProperty_String() {
  EXPORT_TIMESERIES_PROP(std::string, String);
}

void export_TimeSeriesPropertyStatistics() {
  class_<Mantid::Kernel::TimeSeriesPropertyStatistics>(
      "TimeSeriesPropertyStatistics", no_init)
      .add_property("minimum",
                    &Mantid::Kernel::TimeSeriesPropertyStatistics::minimum)
      .add_property("maximum",
                    &Mantid::Kernel::TimeSeriesPropertyStatistics::maximum)
      .add_property("mean", &Mantid::Kernel::TimeSeriesPropertyStatistics::mean)
      .add_property("median",
                    &Mantid::Kernel::TimeSeriesPropertyStatistics::median)
      .add_property(
          "standard_deviation",
          &Mantid::Kernel::TimeSeriesPropertyStatistics::standard_deviation)
      .add_property("time_mean",
                    &Mantid::Kernel::TimeSeriesPropertyStatistics::time_mean)
      .add_property("time_standard_deviation",
                    &Mantid::Kernel::TimeSeriesPropertyStatistics::
                        time_standard_deviation)
      .add_property("duration",
                    &Mantid::Kernel::TimeSeriesPropertyStatistics::duration);
}
