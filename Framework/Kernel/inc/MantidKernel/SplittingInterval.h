// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DateAndTime.h"

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class TimeROI;

/**
 * Class holding a start/end time and a destination for splitting
 * event lists and logs.
 *
 * The start/stop times are saved internally as DateAndTime, for
 * fastest event list splitting.
 *
 * Author: Janik Zikovsky, SNS
 */
class MANTID_KERNEL_DLL SplittingInterval : public TimeInterval {
public:
  /// Default constructor
  SplittingInterval();

  SplittingInterval(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int index = 0);

  int index() const;

  /// @cond
  SplittingInterval operator&(const SplittingInterval &b) const;
  /// @endcond
  SplittingInterval operator|(const SplittingInterval &b) const;
  std::string debugStrPrint() const;

  bool operator==(const SplittingInterval &ti) const;

private:
  /// Index of the destination
  int m_index;
};

/**
 * A typedef for splitting events according their pulse time.
 * It is a vector of SplittingInterval classes.
 *
 */
using SplittingIntervalVec = std::vector<SplittingInterval>;

// -------------- Operators ---------------------
MANTID_KERNEL_DLL SplittingIntervalVec operator+(const SplittingIntervalVec &a, const SplittingIntervalVec &b);
MANTID_KERNEL_DLL SplittingIntervalVec operator&(const SplittingIntervalVec &a, const SplittingIntervalVec &b);
MANTID_KERNEL_DLL SplittingIntervalVec operator|(const SplittingIntervalVec &a, const SplittingIntervalVec &b);
MANTID_KERNEL_DLL SplittingIntervalVec operator~(const SplittingIntervalVec &a);

// -------------- Helper Functions ---------------------
// for every workspace index, create a TimeROI out of its associated spliting intervals
MANTID_KERNEL_DLL std::map<int, TimeROI> timeROIsFromSplitters(const SplittingIntervalVec &splitters);

} // Namespace Kernel
} // Namespace Mantid
