#include "MantidQtAPI/MantidQwtWorkspaceData.h"

#include <boost/math/special_functions/fpclassify.hpp>

/// Calculate absolute minimum and maximum values in a vector. Also find the smallest
/// positive value.
/// @param yvalues :: A vector to use.
/// @param yMin    :: Output minimum value.
/// @param yMax    :: Output maximum value.
/// @param yMinPositive :: Output minimum positive value.
void MantidQwtWorkspaceData::calculateYMinAndMax(const std::vector<double> &yvalues,
                                double &yMin, double &yMax,
                                double &yMinPositive) {

  const double maxDouble = std::numeric_limits<double>::max();
  double curMin = maxDouble;
  double curMinPos = maxDouble;
  double curMax = -maxDouble;
  for(size_t i = 0; i < yvalues.size(); ++i)
  {
    // skip NaNs
    if ((boost::math::isnan)(yvalues[i]) || (boost::math::isinf)(yvalues[i]))
      continue;

    // Update our values as appropriate
    const auto &y = yvalues[i];
    if (y < curMin)
      curMin = yvalues[i];
    if (y < curMinPos && y > 0)
      curMinPos = y;
    if (y > curMax)
      curMax = y;
  }

  // Save the results
  if (curMin == maxDouble) {
    yMin = 0.0;
    yMinPositive = 0.1;
    yMax = 1.0;
    return;
  } else {
    yMin = curMin;
  }

  if (curMax == curMin) {
    curMax *= 1.1;
  }
  yMax = curMax;

  if (curMinPos == maxDouble) {
    yMinPositive = 0.1;
  } else {
    yMinPositive = curMinPos;
  }
}
