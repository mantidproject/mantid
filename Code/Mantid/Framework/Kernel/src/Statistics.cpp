// Includes
#include "MantidKernel/Statistics.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>


namespace Mantid {
namespace Kernel {

using std::string;
using std::vector;

/**
 * Generate a Statistics object where all of the values are NaN. This is a good
 * initial default.
 */
Statistics getNanStatistics() {
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
template <typename TYPE>
double getMedian(const vector<TYPE> &data, const size_t num_data,
                 const bool sorted) {
  if (num_data == 1)
    return static_cast<double>(*(data.begin()));

  bool is_even = ((num_data % 2) == 0);
  if (is_even) {
    double left = 0.0;
    double right = 0.0;

    if (sorted) {
      // Just get the centre two elements.
      left = static_cast<double>(*(data.begin() + num_data / 2 - 1));
      right = static_cast<double>(*(data.begin() + num_data / 2));
    } else {
      // If the data is not sorted, make a copy we can mess with
      vector<TYPE> temp(data.begin(), data.end());
      // Get what the centre two elements should be...
      std::nth_element(temp.begin(), temp.begin() + num_data / 2 - 1,
                       temp.end());
      left = static_cast<double>(*(temp.begin() + num_data / 2 - 1));
      std::nth_element(temp.begin(), temp.begin() + num_data / 2, temp.end());
      right = static_cast<double>(*(temp.begin() + num_data / 2));
    }
    // return the average
    return (left + right) / 2.;
  } else
  // Odd number
  {
    if (sorted) {
      // If sorted and odd, just return the centre value
      return static_cast<double>(*(data.begin() + num_data / 2));
    } else {
      // If the data is not sorted, make a copy we can mess with
      vector<TYPE> temp(data.begin(), data.end());
      // Make sure the centre value is in the correct position
      std::nth_element(temp.begin(), temp.begin() + num_data / 2, temp.end());
      // Now return the centre value
      return static_cast<double>(*(temp.begin() + num_data / 2));
    }
  }
}
/**
 * There are enough special cases in determining the Z score where it useful to
 * put it in a single function.
 */
template <typename TYPE>
std::vector<double> getZscore(const vector<TYPE> &data, const bool sorted) {
  if (data.size() < 3) {
    std::vector<double> Zscore(data.size(), 0.);
    return Zscore;
  }
  std::vector<double> Zscore;

  Statistics stats = getStatistics(data, sorted);
  if (stats.standard_deviation == 0.) {
    std::vector<double> Zscore(data.size(), 0.);
    return Zscore;
  }
  typename vector<TYPE>::const_iterator it = data.begin();
  for (; it != data.end(); ++it) {
    double tmp = static_cast<double>(*it);
    Zscore.push_back(fabs((tmp - stats.mean) / stats.standard_deviation));
  }
  return Zscore;
}
/**
 * There are enough special cases in determining the modified Z score where it
 * useful to
 * put it in a single function.
 */
template <typename TYPE>
std::vector<double> getModifiedZscore(const vector<TYPE> &data,
                                      const bool sorted) {
  if (data.size() < 3) {
    std::vector<double> Zscore(data.size(), 0.);
    return Zscore;
  }
  std::vector<double> MADvec;
  double tmp;
  size_t num_data = data.size(); // cache since it is frequently used
  double median = getMedian(data, num_data, sorted);
  typename vector<TYPE>::const_iterator it = data.begin();
  for (; it != data.end(); ++it) {
    tmp = static_cast<double>(*it);
    MADvec.push_back(fabs(tmp - median));
  }
  double MAD = getMedian(MADvec, num_data, sorted);
  if (MAD == 0.) {
    std::vector<double> Zscore(data.size(), 0.);
    return Zscore;
  }
  MADvec.clear();
  std::vector<double> Zscore;
  it = data.begin();
  for (; it != data.end(); ++it) {
    tmp = static_cast<double>(*it);
    Zscore.push_back(0.6745 * fabs((tmp - median) / MAD));
  }
  return Zscore;
}

/**
 * Determine the statistics for a vector of data. If it is sorted then let the
 * function know so it won't make a copy of the data for determining the median.
 */
template <typename TYPE>
Statistics getStatistics(const vector<TYPE> &data, const bool sorted) {
  Statistics stats = getNanStatistics();
  size_t num_data = data.size(); // cache since it is frequently used

  if (num_data == 0) { // don't do anything
    return stats;
  }

  // calculate the mean
  const TYPE sum = std::accumulate(data.begin(), data.end(),
                                   static_cast<TYPE>(0), std::plus<TYPE>());
  stats.mean = static_cast<double>(sum) / (static_cast<double>(num_data));

  // calculate the standard deviation, min, max
  stats.minimum = stats.mean;
  stats.maximum = stats.mean;
  double stddev = 0.;
  typename vector<TYPE>::const_iterator it = data.begin();
  for (; it != data.end(); ++it) {
    double temp = static_cast<double>(*it);
    stddev += ((temp - stats.mean) * (temp - stats.mean));
    if (temp > stats.maximum)
      stats.maximum = temp;
    if (temp < stats.minimum)
      stats.minimum = temp;
  }
  stats.standard_deviation = sqrt(stddev / (static_cast<double>(num_data)));

  // calculate the median
  stats.median = getMedian(data, num_data, sorted);

  return stats;
}

/// Getting statistics of a string array should just give a bunch of NaNs
template <>
DLLExport Statistics
    getStatistics<string>(const vector<string> &data, const bool sorted) {
  UNUSED_ARG(sorted);
  UNUSED_ARG(data);
  return getNanStatistics();
}

/// Getting statistics of a boolean array should just give a bunch of NaNs
template <>
DLLExport Statistics
    getStatistics<bool>(const vector<bool> &data, const bool sorted) {
  UNUSED_ARG(sorted);
  UNUSED_ARG(data);
  return getNanStatistics();
}

/** Return the Rwp of a diffraction pattern data
  * @param obsI :: array of observed intensity values
  * @param calI :: array of calculated intensity values;
  * @param obsE :: array of error of the observed data;
  * @return :: RFactor including Rp and Rwp
  *
  */
Rfactor getRFactor(const std::vector<double> &obsI,
                   const std::vector<double> &calI,
                   const std::vector<double> &obsE) {
  // 1. Check
  if (obsI.size() != calI.size() || obsI.size() != obsE.size()) {
    std::stringstream errss;
    errss << "GetRFactor() Input Error!  Observed Intensity (" << obsI.size()
          << "), Calculated Intensity (" << calI.size()
          << ") and Observed Error (" << obsE.size()
          << ") have different number of elements.";
    throw std::runtime_error(errss.str());
  }
  if (obsI.size() == 0) {
    throw std::runtime_error("getRFactor(): the input arrays are empty.");
  }

  double sumnom = 0;
  double sumdenom = 0;
  double sumrpnom = 0;
  double sumrpdenom = 0;

  size_t numpts = obsI.size();
  for (size_t i = 0; i < numpts; ++i) {
    double cal_i = calI[i];
    double obs_i = obsI[i];
    double sigma = obsE[i];
    double weight = 1.0 / (sigma * sigma);
    double diff = obs_i - cal_i;

    if (weight == weight && weight <= DBL_MAX) {
      // If weight is not NaN.
      sumrpnom += fabs(diff);
      sumrpdenom += fabs(obs_i);

      double tempnom = weight * diff * diff;
      double tempden = weight * obs_i * obs_i;

      sumnom += tempnom;
      sumdenom += tempden;

      if (tempnom != tempnom || tempden != tempden) {
        std::cout << "***** Error! ****** Data indexed " << i << " is NaN. "
                  << "i = " << i << ": cal = " << calI[i] << ", obs = " << obs_i
                  << ", weight = " << weight << ". \n";
      }
    }
  }

  Rfactor rfactor(0., 0.);
  rfactor.Rp = (sumrpnom / sumrpdenom);
  rfactor.Rwp = std::sqrt(sumnom / sumdenom);

  if (rfactor.Rwp != rfactor.Rwp)
    std::cout << "Rwp is NaN.  Denominator = " << sumnom
              << "; Nominator = " << sumdenom << ". \n";

  return rfactor;
}

/**
 * This will calculate the first n-moments (inclusive) about the origin. For
 *example
 * if maxMoment=2 then this will return 3 values: 0th (total weight), 1st
 *(mean), 2nd (deviation).
 *
 * @param x The independent values
 * @param y The dependent values
 * @param maxMoment The number of moments to calculate
 * @returns The first n-moments.
 */
template <typename TYPE>
std::vector<double> getMomentsAboutOrigin(const std::vector<TYPE> &x,
                                          const std::vector<TYPE> &y,
                                          const int maxMoment) {
  // densities have the same number of x and y
  bool isDensity(x.size() == y.size());

  // if it isn't a density then check for histogram
  if ((!isDensity) && (x.size() != y.size() + 1)) {
    std::stringstream msg;
    msg << "length of x (" << x.size() << ") and y (" << y.size()
        << ")do not match";
    throw std::out_of_range(msg.str());
  }

  // initialize a result vector with all zeros
  std::vector<double> result(maxMoment + 1, 0.);

  // cache the maximum index
  size_t numPoints = y.size();
  if (isDensity)
    numPoints = x.size() - 1;

  // densities are calculated using Newton's method for numerical integration
  // as backwards as it sounds, the outer loop should be the points rather than
  // the moments
  for (size_t j = 0; j < numPoints; ++j) {
    // reduce item lookup - and central x for histogram
    const double xVal = .5 * static_cast<double>(x[j] + x[j + 1]);
    // this variable will be (x^n)*y
    double temp = static_cast<double>(y[j]); // correct for histogram
    if (isDensity) {
      const double xDelta = static_cast<double>(x[j + 1] - x[j]);
      temp = .5 * (temp + static_cast<double>(y[j + 1])) * xDelta;
    }

    // accumulate the moments
    result[0] += temp;
    for (size_t i = 1; i < result.size(); ++i) {
      temp *= xVal;
      result[i] += temp;
    }
  }

  return result;
}

/**
 * This will calculate the first n-moments (inclusive) about the mean (1st
 *moment). For example
 * if maxMoment=2 then this will return 3 values: 0th (total weight), 1st
 *(mean), 2nd (deviation).
 *
 * @param x The independent values
 * @param y The dependent values
 * @param maxMoment The number of moments to calculate
 * @returns The first n-moments.
 */
template <typename TYPE>
std::vector<double> getMomentsAboutMean(const std::vector<TYPE> &x,
                                        const std::vector<TYPE> &y,
                                        const int maxMoment) {
  // get the zeroth (integrated value) and first moment (mean)
  std::vector<double> momentsAboutOrigin = getMomentsAboutOrigin(x, y, 1);
  const double mean = momentsAboutOrigin[1];

  // initialize a result vector with all zeros
  std::vector<double> result(maxMoment + 1, 0.);
  result[0] = momentsAboutOrigin[0];

  // escape early if we need to
  if (maxMoment == 0)
    return result;

  // densities have the same number of x and y
  bool isDensity(x.size() == y.size());

  // cache the maximum index
  size_t numPoints = y.size();
  if (isDensity)
    numPoints = x.size() - 1;

  // densities are calculated using Newton's method for numerical integration
  // as backwards as it sounds, the outer loop should be the points rather than
  // the moments
  for (size_t j = 0; j < numPoints; ++j) {
    // central x in histogram with a change of variables - and just change for
    // density
    const double xVal =
        .5 * static_cast<double>(x[j] + x[j + 1]) - mean; // change of variables

    // this variable will be (x^n)*y
    double temp;
    if (isDensity) {
      const double xDelta = static_cast<double>(x[j + 1] - x[j]);
      temp = xVal * .5 * static_cast<double>(y[j] + y[j + 1]) * xDelta;
    } else {
      temp = xVal * static_cast<double>(y[j]);
    }

    // accumulate the moment
    result[1] += temp;
    for (size_t i = 2; i < result.size(); ++i) {
      temp *= xVal;
      result[i] += temp;
    }
  }

  return result;
}

// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE)                                                      \
  template MANTID_KERNEL_DLL Statistics                                        \
      getStatistics<TYPE>(const vector<TYPE> &, const bool);                   \
  template MANTID_KERNEL_DLL std::vector<double> getZscore<TYPE>(              \
      const vector<TYPE> &, const bool);                                       \
  template MANTID_KERNEL_DLL std::vector<double> getModifiedZscore<TYPE>(      \
      const vector<TYPE> &, const bool);                                       \
  template MANTID_KERNEL_DLL std::vector<double> getMomentsAboutOrigin<TYPE>(  \
      const std::vector<TYPE> &x, const std::vector<TYPE> &y,                  \
      const int maxMoment);                                                    \
  template MANTID_KERNEL_DLL std::vector<double> getMomentsAboutMean<TYPE>(    \
      const std::vector<TYPE> &x, const std::vector<TYPE> &y,                  \
      const int maxMoment);

// --------------------------- Concrete instantiations
// ---------------------------------------------
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATE(int)
INSTANTIATE(long)
INSTANTIATE(long long)
INSTANTIATE(unsigned int)
INSTANTIATE(unsigned long)
INSTANTIATE(unsigned long long)

} // namespace Kernel
} // namespace Mantid
