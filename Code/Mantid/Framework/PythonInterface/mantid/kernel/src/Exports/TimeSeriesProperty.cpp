#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/implicit.hpp>

using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::Property;
using namespace boost::python;

namespace
{
  /// Defines the getStatistics member for various types
  #define DEF_GET_STATS(TYPE) DEF_GET_STATS_##TYPE
  /// Doubles have a get stats member
  #define DEF_GET_STATS_double .def("getStatistics", &TimeSeriesProperty<double>::getStatistics)
  /// booleans do not have a get stats member
  #define DEF_GET_STATS_bool 

  /// Macro to reduce copy-and-paste
  #define EXPORT_TIMESERIES_PROP(TYPE, Prefix)\
    register_ptr_to_python<TimeSeriesProperty<TYPE>*>();\
    register_ptr_to_python<const TimeSeriesProperty<TYPE>*>();\
    implicitly_convertible<TimeSeriesProperty<TYPE>*,const TimeSeriesProperty<TYPE>*>();\
    \
    class_<TimeSeriesProperty<TYPE>, bases<Property>, boost::noncopyable>(#Prefix"TimeSeriesProperty", no_init)\
      .add_property("value", &Mantid::Kernel::TimeSeriesProperty<TYPE>::valuesAsVector) \
      .add_property("times", &Mantid::Kernel::TimeSeriesProperty<TYPE>::timesAsVector) \
      .def("valueAsString", &TimeSeriesProperty<TYPE>::value) \
      .def("size", &TimeSeriesProperty<TYPE>::size)\
      .def("firstTime", &TimeSeriesProperty<TYPE>::firstTime) \
      .def("firstValue", &TimeSeriesProperty<TYPE>::firstValue) \
      .def("lastTime", &TimeSeriesProperty<TYPE>::lastTime) \
      .def("lastValue", &TimeSeriesProperty<TYPE>::lastValue) \
      .def("nthValue", &TimeSeriesProperty<TYPE>::nthValue) \
      .def("nthTime", &TimeSeriesProperty<TYPE>::nthTime) \
      DEF_GET_STATS(TYPE) \
      ;
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
