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

/// Copy Constructor
SplittingInterval::SplittingInterval(const SplittingInterval& other) :
    m_start(other.m_start), m_stop(other.m_stop), m_index(other.m_index)
{
}

/// Return the start time
PulseTimeType SplittingInterval::start() const
{
  return m_start;
}

/// Return the stop time
PulseTimeType SplittingInterval::stop() const
{
  return m_stop;
}

/// Return the start time
dateAndTime SplittingInterval::startDate() const
{
  return DateAndTime::get_time_from_pulse_time(m_start);
}

/// Return the stop time
dateAndTime SplittingInterval::stopDate() const
{
  return DateAndTime::get_time_from_pulse_time(m_stop);
}

/// Returns the duration in seconds
double SplittingInterval::duration() const
{
  return DateAndTime::durationInSeconds( stopDate() - startDate() );
}

/// Return the index (destination of this split time block)
int SplittingInterval::index() const
{
  return m_index;
}

/// Return true if the b SplittingInterval overlaps with this one.
double SplittingInterval::overlaps(const SplittingInterval& b) const
{
  return ((b.m_start < this->m_stop) && (b.m_start >= this->m_start))
      || ((b.m_stop < this->m_stop) && (b.m_stop >= this->m_start))
      || ((this->m_start < b.m_stop) && (this->m_start >= b.m_start))
      || ((this->m_stop < b.m_stop) && (this->m_stop >= b.m_start))
      ;
}

/// And operator. Return the smallest time interval where both intervals are TRUE.
SplittingInterval SplittingInterval::operator &(const SplittingInterval& b) const
{
  SplittingInterval out(*this);
  if (b.m_start > this->m_start)
    out.m_start = b.m_start;
  if (b.m_stop < this->m_stop)
    out.m_stop = b.m_stop;
  return out;
}

SplittingInterval SplittingInterval::operator |(const SplittingInterval& b) const
{
  SplittingInterval out(*this);
  if (!this->overlaps(b))
    throw std::invalid_argument("SplittingInterval: cannot apply the OR (|) operator to non-overlapping SplittingInterval's.");

  if (b.m_start < this->m_start)
    out.m_start = b.m_start;
  if (b.m_stop > this->m_stop)
    out.m_stop = b.m_stop;
  return out;
}


/** Comparator for sorting lists of SplittingInterval */
bool compareSplittingInterval(const SplittingInterval & si1, const SplittingInterval & si2)
{
  return (si1.start() < si2.start());
}

//------------------------------------------------------------------------------------------------
/** Return true if the TimeSplitterType provided is a filter,
 * meaning that it only has an output index of 0.
 */
bool isFilter(const TimeSplitterType& a)
{
  int max=-1;
  TimeSplitterType::const_iterator it;
  for (it = a.begin(); it!=a.end(); it++)
    if (it->index() > max)
      max = it->index();
  return (max <= 0);
}

//------------------------------------------------------------------------------------------------
/** Plus operator for TimeSplitterType.
 * Combines a filter and a splitter by removing entries that are filtered out from the splitter.
 * Combines two filters together by "and"ing them
 */
TimeSplitterType operator +(const TimeSplitterType& a, const TimeSplitterType& b)
{
  TimeSplitterType out;

  return out;
}


//------------------------------------------------------------------------------------------------
/** AND operator for TimeSplitterType
 * Only works on Filters, not splitters. Combines the splitters
 * to only keep times where both Filters are TRUE.
 */
TimeSplitterType operator &(const TimeSplitterType& a, const TimeSplitterType& b)
{
  TimeSplitterType out;
  //If either is empty, then no entries in the filter (aka everything is removed)
  if ((a.size()==0) || (b.size()==0))
    return out;

  TimeSplitterType::const_iterator ait;
  TimeSplitterType::const_iterator bit;

  //For now, a simple double iteration. Can be made smarter if a and b are sorted.
  for (ait=a.begin(); ait != a.end(); ait++)
  {
    for (bit=b.begin(); bit != b.end(); bit++)
    {
      if (ait->overlaps(*bit))
      {
        out.push_back( *ait & *bit );
      }
    }
  }
  return out;
}


//------------------------------------------------------------------------------------------------
/** OR operator for TimeSplitterType
 * Only works on Filters, not splitters. Combines the splitters
 * to only keep times where EITHER Filter is TRUE.
 */
TimeSplitterType operator |(const TimeSplitterType& a, const TimeSplitterType& b)
{
  TimeSplitterType out;

  //Concatenate the two lists
  TimeSplitterType temp = a;
  temp.insert(temp.end(), b.begin(), b.end());

  //Sort by start time
  std::sort(temp.begin(), temp.end(), compareSplittingInterval);

  //Now we have to merge duplicate/overlapping intervals together
  TimeSplitterType::iterator it = temp.begin();
  while (it != temp.end())
  {
    //All following intervals will start at or after this one
    PulseTimeType start = it->start();
    PulseTimeType stop = it->stop();

    //Keep looking for the next interval where there is a gap (start > old stop);
    while ((it != temp.end()) && (it->start() <= stop))
    {
      //Extend the stop point (the start cannot be extended since the list is sorted)
      if (it->stop() > stop)
        stop = it->stop();
      it++;
    }
    //We've reached a gap point. Output this merged interval and move on.
    out.push_back( SplittingInterval(start, stop, 0) );
  }

  return out;
}


//------------------------------------------------------------------------------------------------
/** NOT operator for TimeSplitterType
 * Only works on Filters. Returns a filter with the reversed
 * time intervals as the incoming filter.
 */
TimeSplitterType operator ~(const TimeSplitterType& a)
{
  TimeSplitterType out;
  return out;
}

}
}
