#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::Property;
using namespace boost::python;

void export_TimeSeriesProperty_Double()
{
  class_<TimeSeriesProperty<double>, bases<Property>, boost::noncopyable>("TimeSeriesProperty_dbl", no_init)
    .def("getStatistics", &Mantid::Kernel::TimeSeriesProperty<double>::getStatistics)
    .add_property("value", &Mantid::Kernel::TimeSeriesProperty<double>::valuesAsVector)
    .add_property("times", &Mantid::Kernel::TimeSeriesProperty<double>::timesAsVector)
  ;
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
