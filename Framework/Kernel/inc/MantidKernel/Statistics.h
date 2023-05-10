// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Kernel {
namespace Math {
/**
 * Maps a "statistic" to a number
 */
enum StatisticType : int {
  FirstValue,
  LastValue,
  Minimum,
  Maximum,
  Mean,
  TimeAveragedMean,
  Median,
  StdDev,
  TimeAverageStdDev
};
} // namespace Math

/**
   Simple struct to store statistics.

 */
struct MANTID_KERNEL_DLL Statistics {
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

  /// Default value for everything is nan
  Statistics();
};

/// Controls the computation of statisical data
struct StatOptions {
  enum Flag {
    SortedData = 1,        // is the data sorted?
    Mean = 2,              // calculate the mean
    UncorrectedStdDev = 4, // calculate the s.d. using N dofs
    CorrectedStdDev = 8,   // calculate the s.d. using N-1 dofs
    Median = 16,           // calculate the median
    AllStats = (Mean | UncorrectedStdDev | Median)
  };
};

/** R factor for powder data analysis
 */
struct Rfactor {
  /// Rwp
  double Rwp;
  /// Rp
  double Rp;
  ///  Foos(int a, int b) : a(a), b(b) {}
  Rfactor(double rwp, double rp) : Rwp(rwp), Rp(rp) {}
  Rfactor() {
    Rwp = -1.0;
    Rp = -1.0;
  }
};

/// Return a statistics object for the given data set
template <typename TYPE>
Statistics getStatistics(const std::vector<TYPE> &data, const unsigned int flags = StatOptions::AllStats);
/// Return the Z score values for a dataset
template <typename TYPE> std::vector<double> getZscore(const std::vector<TYPE> &data);
template <typename TYPE>
std::vector<double> getWeightedZscore(const std::vector<TYPE> &data, const std::vector<TYPE> &weights);
/// Return the modified Z score values for a dataset
template <typename TYPE> std::vector<double> getModifiedZscore(const std::vector<TYPE> &data);
/// Return the R-factors (Rwp) of a diffraction pattern data
Rfactor MANTID_KERNEL_DLL getRFactor(const std::vector<double> &obsI, const std::vector<double> &calI,
                                     const std::vector<double> &obsE);

/// Return the first n-moments of the supplied data.
template <typename TYPE>
std::vector<double> getMomentsAboutOrigin(const std::vector<TYPE> &x, const std::vector<TYPE> &y,
                                          const int maxMoment = 3);
/// Return the first n-moments of the supplied data.
template <typename TYPE>
std::vector<double> getMomentsAboutMean(const std::vector<TYPE> &x, const std::vector<TYPE> &y,
                                        const int maxMoment = 3);
} // namespace Kernel
} // namespace Mantid
