#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include <ctime>
#include <ostream>

namespace Mantid
{
namespace Kernel
{

/// Default constructor
SplittingInterval::SplittingInterval() :
    m_start(), m_stop(), m_index(-1)
{

}

/// Constructor using dateAndTime
SplittingInterval::SplittingInterval(const dateAndTime& start, const dateAndTime& stop, const int index) :
    m_start( DateAndTime::get_from_absolute_time(start) ), m_stop( DateAndTime::get_from_absolute_time(stop) ), m_index(index)
{

}

/// Constructor using PulseTimeType
SplittingInterval::SplittingInterval(const PulseTimeType& start, const PulseTimeType& stop, const int index) :
    m_start(start), m_stop(stop), m_index(index)
{
}

/// Return the start time
PulseTimeType SplittingInterval::start()
{
  return m_start;
}

/// Return the stop time
PulseTimeType SplittingInterval::stop()
{
  return m_stop;
}

/// Return the start time
dateAndTime SplittingInterval::startDate()
{
  return DateAndTime::get_time_from_pulse_time(m_start);
}

/// Return the stop time
dateAndTime SplittingInterval::stopDate()
{
  return DateAndTime::get_time_from_pulse_time(m_stop);
}

/// Return the index (destination of this split time block)
int SplittingInterval::index()
{
  return m_index;
}


}
}
