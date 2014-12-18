#ifndef TIMESPLITTER_H
#define TIMESPLITTER_H

#include "MantidKernel/DateAndTime.h"

namespace Mantid {
namespace Kernel {

/**
 * Class holding a start/end time and a destination for splitting
 * event lists and logs.
 *
 * The start/stop times are saved internally as DateAndTime, for
 * fastest event list splitting.
 *
 * Author: Janik Zikovsky, SNS
 */
class MANTID_KERNEL_DLL SplittingInterval {
public:
  /// Default constructor
  SplittingInterval();

  SplittingInterval(const SplittingInterval &other);

  SplittingInterval(const DateAndTime &start, const DateAndTime &stop,
                    const int index = 0);

  DateAndTime start() const;
  DateAndTime stop() const;

  double duration() const;

  int index() const;

  bool overlaps(const SplittingInterval &b) const;

  SplittingInterval operator&(const SplittingInterval &b) const;

  SplittingInterval operator|(const SplittingInterval &b) const;

  bool operator<(const SplittingInterval &b) const;
  bool operator>(const SplittingInterval &b) const;

private:
  /// begin
  DateAndTime m_start;
  /// end
  DateAndTime m_stop;
  /// Index of the destination
  int m_index;
};

/**
 * A typedef for splitting events according their pulse time.
 * It is a vector of SplittingInterval classes.
 *
 */
typedef std::vector<SplittingInterval> TimeSplitterType;

// -------------- Operators ---------------------
MANTID_KERNEL_DLL TimeSplitterType
operator+(const TimeSplitterType &a, const TimeSplitterType &b);
MANTID_KERNEL_DLL TimeSplitterType
operator&(const TimeSplitterType &a, const TimeSplitterType &b);
MANTID_KERNEL_DLL TimeSplitterType
operator|(const TimeSplitterType &a, const TimeSplitterType &b);
MANTID_KERNEL_DLL TimeSplitterType operator~(const TimeSplitterType &a);

} // Namespace Kernel
} // Namespace Mantid

#endif // TIMESPLITTER_H
