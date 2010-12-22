#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <vector>
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

struct Statistics
{
  /// Minimum value
  double minimum;
  /// Maximum value
  double maximum;
  /// Mean value
  double mean;
  /// Median value
  double median;
  /// standard_deviation of the values
  double standard_deviation;
};

template<typename TYPE>
Statistics getStatistics(const std::vector<TYPE>& data, const bool sorted=false);

} // namespace Kernel
} // namespace Mantid
#endif /* STATISTICS_H_ */
