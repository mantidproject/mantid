#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/init.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::DateAndTime;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::Property;
using namespace boost::python;

namespace
{

  using Mantid::PythonInterface::Policies::VectorToNumpy;

  // Macro to reduce copy-and-paste
  #define EXPORT_TIMESERIES_PROP(TYPE, Prefix)\
    register_ptr_to_python<TimeSeriesProperty<TYPE>*>();\
    \
    class_<TimeSeriesProperty<TYPE>, bases<Property>, boost::noncopyable>(#Prefix"TimeSeriesProperty", init<const std::string&>())\
      .add_property("value", make_function(&Mantid::Kernel::TimeSeriesProperty<TYPE>::valuesAsVector, return_value_policy<VectorToNumpy>())) \
      .add_property("times", &Mantid::Kernel::TimeSeriesProperty<TYPE>::timesAsVector) \
      .def("addValue", (void (TimeSeriesProperty<TYPE>::*)(const DateAndTime&,const TYPE))&TimeSeriesProperty<TYPE>::addValue) \
      .def("addValue", (void (TimeSeriesProperty<TYPE>::*)(const std::string&,const TYPE))&TimeSeriesProperty<TYPE>::addValue) \
      .def("valueAsString", &TimeSeriesProperty<TYPE>::value) \
      .def("size", &TimeSeriesProperty<TYPE>::size)\
      .def("firstTime", &TimeSeriesProperty<TYPE>::firstTime) \
      .def("firstValue", &TimeSeriesProperty<TYPE>::firstValue) \
      .def("lastTime", &TimeSeriesProperty<TYPE>::lastTime) \
      .def("lastValue", &TimeSeriesProperty<TYPE>::lastValue) \
      .def("nthValue", &TimeSeriesProperty<TYPE>::nthValue) \
      .def("nthTime", &TimeSeriesProperty<TYPE>::nthTime) \
      .def("getStatistics", &TimeSeriesProperty<TYPE>::getStatistics) \
      .def("timeAverageValue", &TimeSeriesProperty<TYPE>::timeAverageValue) \
      ;
}

void export_TimeSeriesProperty_Double()
{
  EXPORT_TIMESERIES_PROP(double, Float);
}

void export_TimeSeriesProperty_Bool()
{
  EXPORT_TIMESERIES_PROP(bool, Bool);
}

void export_TimeSeriesProperty_Int32()
{
  EXPORT_TIMESERIES_PROP(int32_t, Int32);
}

void export_TimeSeriesProperty_Int64()
{
  EXPORT_TIMESERIES_PROP(int64_t, Int64);
}

void export_TimeSeriesProperty_String()
{
  EXPORT_TIMESERIES_PROP(std::string, String);
}


void export_TimeSeriesPropertyStatistics()
{
  class_<Mantid::Kernel::TimeSeriesPropertyStatistics>("TimeSeriesPropertyStatistics", no_init)
    .add_property("minimum", &Mantid::Kernel::TimeSeriesPropertyStatistics::minimum)
    .add_property("maximum", &Mantid::Kernel::TimeSeriesPropertyStatistics::maximum)
    .add_property("mean", &Mantid::Kernel::TimeSeriesPropertyStatistics::mean)
    .add_property("median", &Mantid::Kernel::TimeSeriesPropertyStatistics::median)
    .add_property("standard_deviation", &Mantid::Kernel::TimeSeriesPropertyStatistics::standard_deviation)
    .add_property("duration", &Mantid::Kernel::TimeSeriesPropertyStatistics::duration)
  ;
}
