#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Exception.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <numeric>
#include <vector>

namespace Mantid
{
namespace Kernel
{

  //Logger& TimeSeriesProperty::g_log = Logger::get("TimeSeriesProperty");

/// @cond

template<class TYPE>
TimeSeriesPropertyStatistics TimeSeriesProperty<TYPE>::getStatistics()
{
  throw Exception::NotImplementedError("Cannot calculate statistics for this type");
}


template<>
TimeSeriesPropertyStatistics TimeSeriesProperty<double>::getStatistics()
{

  TimeSeriesPropertyStatistics out;
  int num = static_cast<int>(this->size());
  if (num <= 0)
    return out;

  std::map<dateAndTime, double> map = this->valueAsCorrectMap();
  std::map<dateAndTime, double>::iterator it;

  //First you need the mean
  int counter = 0;
  double total = 0.;
  double min = std::numeric_limits<double>::max();
  double max = -min;
  for (it = map.begin(); it != map.end(); it++)
  {
    double val = it->second;
    //We will also find the limits and median at the same time.
    if (val < min) min = val;
    if (val > max) max = val;
    if (counter == num/2)
      out.median = val;
    total += val;
    counter++;
  }
  out.maximum=max;
  out.minimum=min;

  //Now the mean
  double mean = total/num;
  out.mean = mean;

  //Duration: the last - the first times
  out.duration = DateAndTime::durationInSeconds(  map.rbegin()->first - map.begin()->first  );

  //Now the standard deviation
  total = 0;
  for (it = map.begin(); it != map.end(); it++)
  {
    double val = (it->second-mean);
    total += val*val;
  }
  out.standard_deviation = sqrt( double( total / num ) );

  return out;
}


template DLLExport class TimeSeriesProperty<double>;
template DLLExport class TimeSeriesProperty<std::string>;

/// @endcond

} // namespace Kernel
} // namespace Mantid


