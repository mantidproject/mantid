#include <algorithm>
#include <functional>
#include <limits>
#include <math.h>
#include <numeric>
#include <string>
#include "MantidKernel/Statistics.h"

namespace Mantid
{
namespace Kernel
{

using std::string;
using std::vector;

/**
 * Generate a Statistics object where all of the values are NaN. This is a good initial default.
 */
Statistics getNanStatistics()
{
  double nan = std::numeric_limits<double>::quiet_NaN();

  Statistics stats;
  stats.minimum = nan;
  stats.maximum = nan;
  stats.mean = nan;
  stats.median = nan;
  stats.standard_deviation = nan;

  return stats;
}

/**
 * There are enough special cases in determining the median where it useful to
 * put it in a single function.
 */
template<typename TYPE>
double getMedian(const vector<TYPE>& data, const size_t num_data)
{
  if (num_data == 1)
    return static_cast<double>(*(data.begin()));

  bool is_even = ((num_data % 2) == 0);
  if (is_even) {
    double left = static_cast<double>(*(data.begin() + num_data/2 - 1));
    double right = static_cast<double>(*(data.begin() + num_data/2));
    return (left + right) / 2.;
  }
  else {
    return static_cast<double>(*(data.begin() + num_data/2));
  }
}

/**
 * Determine the statistics for a vector of data. If it is sorted then let the
 * function know so it won't make a copy of the data for determining the median.
 */
template<typename TYPE>
Statistics getStatistics(const vector<TYPE>& data, const bool sorted)
{
  Statistics stats = getNanStatistics();
  size_t num_data = data.size(); // chache since it is frequently used

  if (num_data == 0) { // don't do anything
    return stats;
  }

  // calculate the mean
  stats.mean = std::accumulate(data.begin(), data.end(), 0., std::plus<double>());
  stats.mean /= (static_cast<double>(num_data));

  // calculate the standard deviation, min, max
  stats.minimum = stats.mean;
  stats.maximum = stats.mean;
  double stddev = 0.;
  double temp;
  typename vector<TYPE>::const_iterator it = data.begin();
  for ( ; it != data.end(); ++it)
  {
    temp = static_cast<double>(*it);
    stddev += ((temp-stats.mean)* (temp-stats.mean));
    if (temp > stats.maximum)
      stats.maximum = temp;
    if (temp < stats.minimum)
      stats.minimum = temp;
  }
  stats.standard_deviation = sqrt(stddev / (static_cast<double>(num_data)));

  // calculate the median
  if (sorted) {
    stats.median = getMedian(data, num_data);
  }
  else {
    vector<TYPE> temp(data.begin(), data.end());
    std::nth_element(temp.begin(), temp.begin()+num_data/2, temp.end());
    stats.median = getMedian(temp, num_data);
  }

  return stats;
}

/// Getting statistics of a string array should just give a bunch of NaNs
template<>
Statistics getStatistics<string>(const vector<string>& data, const bool sorted)
{
  return getNanStatistics();
}

// -------------------------- concrete instantiations
template DLLExport Statistics getStatistics<double>(const vector<double> &, const bool);
template DLLExport Statistics getStatistics<int32_t>(const vector<int32_t> &, const bool);

} // namespace Kernel
} // namespace Mantid
