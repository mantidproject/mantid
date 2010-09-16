#ifndef TIMESPLITTER_H
#define TIMESPLITTER_H

#include "MantidKernel/DateAndTime.h"
#include <ctime>
#include <ostream>

namespace Mantid
{
namespace Kernel
{


/**
 * Class holding a start/end time and a destination for splitting
 * event lists and logs.
 *
 * The start/stop times are saved internally as PulseTimeType, for
 * fastest event list splitting.
 *
 * Author: Janik Zikovsky, SNS
 */
class DLLExport SplittingInterval
{
public:
  /// Default constructor
  SplittingInterval();

  /// Constructor using dateAndTime
  SplittingInterval(const dateAndTime& start, const dateAndTime& stop, const int index);

  /// Constructor using PulseTimeType
  SplittingInterval(const PulseTimeType& start, const PulseTimeType& stop, const int index);

  PulseTimeType start();
  PulseTimeType stop();

  dateAndTime startDate();
  dateAndTime stopDate();

  int index();

private:
    /// begin
    PulseTimeType m_start;
    /// end
    PulseTimeType m_stop;
    /// Index of the destination
    int m_index;
};


/**
 * A typedef for splitting events according their pulse time.
 * It is a vector of SplittingInterval classes.
 *
 */
typedef std::vector< SplittingInterval > TimeSplitterType;

} //Namespace Kernel
} //Namespace Mantid

#endif // TIMESPLITTER_H


