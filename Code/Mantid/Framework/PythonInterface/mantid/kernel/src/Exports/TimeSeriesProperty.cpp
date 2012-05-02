#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/implicit.hpp>

using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::Property;
using namespace boost::python;

void export_TimeSeriesProperty_Double()
{
  register_ptr_to_python<TimeSeriesProperty<double>*>();
  register_ptr_to_python<const TimeSeriesProperty<double>*>();
  implicitly_convertible<TimeSeriesProperty<double>*,const TimeSeriesProperty<double>*>();

  class_<TimeSeriesProperty<double>, bases<Property>, boost::noncopyable>("FloatTimeSeriesProperty", no_init)
    .def("getStatistics", &Mantid::Kernel::TimeSeriesProperty<double>::getStatistics)
    .add_property("value", &Mantid::Kernel::TimeSeriesProperty<double>::valuesAsVector)
    .add_property("times", &Mantid::Kernel::TimeSeriesProperty<double>::timesAsVector)
  ;
}

void export_TimeSeriesProperty_Bool()
{
  register_ptr_to_python<TimeSeriesProperty<bool>*>();
  register_ptr_to_python<const TimeSeriesProperty<bool>*>();
  implicitly_convertible<TimeSeriesProperty<bool>*,const TimeSeriesProperty<bool>*>();

  class_<TimeSeriesProperty<bool>, bases<Property>, boost::noncopyable>("BoolTimeSeriesProperty", no_init);
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
