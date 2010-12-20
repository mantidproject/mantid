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

/// Constructor using DateAndTime
SplittingInterval::SplittingInterval(const DateAndTime& start, const DateAndTime& stop, const int index) :
    m_start(start), m_stop(stop), m_index(index)
{
}

/// Copy Constructor
SplittingInterval::SplittingInterval(const SplittingInterval& other) :
    m_start(other.m_start), m_stop(other.m_stop), m_index(other.m_index)
{
}

/// Return the start time
DateAndTime SplittingInterval::start() const
{
  return m_start;
}

/// Return the stop time
DateAndTime SplittingInterval::stop() const
{
  return m_stop;
}

/// Returns the duration in seconds
double SplittingInterval::duration() const
{
  return DateAndTime::seconds_from_duration( m_stop - m_start );
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

/// Or operator. Return the largest time interval.
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
 * Also, will combine two filters together by "and"ing them
 *
 * @param a TimeSplitterType splitter OR filter
 * @param b TimeSplitterType splitter OR filter.
 * @throw std::invalid_argument if two splitters are given.
 */
TimeSplitterType operator +(const TimeSplitterType& a, const TimeSplitterType& b)
{
  bool a_filter, b_filter;
  a_filter = isFilter(a);
  b_filter = isFilter(b);

  if (a_filter && b_filter)
  {
    return a & b;
  }
  else if (a_filter && !b_filter)
  {
    return b & a;
  }
  else if (!a_filter && b_filter)
  {
    return a & b;
  }
  else // (!a_filter && !b_filter)
  {
    //Both are splitters.
    throw std::invalid_argument("Cannot combine two splitters together, as the output is undefined. Try splitting each output workspace by b after the a split has been done.");
  }
}



//------------------------------------------------------------------------------------------------
/** AND operator for TimeSplitterType
 * Works on Filters - combines them to only keep times where both Filters are TRUE.
 * Works on splitter + filter if (a) is a splitter and b is a filter.
 *  In general, use the + operator since it will resolve the order for you.
 *
 * @param a TimeSplitterType filter or Splitter.
 * @param b TimeSplitterType filter.
 * @return the ANDed filter
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
        // The & operator for SplittingInterval keeps the index of the left-hand-side (ait in this case)
        //  meaning that a has to be the splitter because the b index is ignored.
        out.push_back( *ait & *bit );
      }
    }
  }
  return out;
}


//------------------------------------------------------------------------------------------------
/** Remove any overlap in a filter (will not work properly on a splitter)
 *
 * @param a TimeSplitterType filter.
 */
TimeSplitterType removeFilterOverlap(const TimeSplitterType &a)
{
  TimeSplitterType out;

  //Now we have to merge duplicate/overlapping intervals together
  TimeSplitterType::const_iterator it = a.begin();
  while (it != a.end())
  {
    //All following intervals will start at or after this one
    DateAndTime start = it->start();
    DateAndTime stop = it->stop();

    //Keep looking for the next interval where there is a gap (start > old stop);
    while ((it != a.end()) && (it->start() <= stop))
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
/** OR operator for TimeSplitterType
 * Only works on Filters, not splitters. Combines the splitters
 * to only keep times where EITHER Filter is TRUE.
 *
 * @param a TimeSplitterType filter.
 * @param b TimeSplitterType filter.
 * @return the ORed filter
 */
TimeSplitterType operator |(const TimeSplitterType& a, const TimeSplitterType& b)
{
  TimeSplitterType out;

  //Concatenate the two lists
  TimeSplitterType temp;
  //temp.insert(temp.end(), b.begin(), b.end());

  // Add the intervals, but don't add any invalid (empty) ranges
  TimeSplitterType::const_iterator it;;
  for (it = a.begin(); it != a.end(); it++)
    if (it->stop() > it->start())
      temp.push_back(*it);
  for (it = b.begin(); it != b.end(); it++)
    if (it->stop() > it->start())
      temp.push_back(*it);

  //Sort by start time
  std::sort(temp.begin(), temp.end(), compareSplittingInterval);

  out = removeFilterOverlap(temp);

  return out;
}


//------------------------------------------------------------------------------------------------
/** NOT operator for TimeSplitterType
 * Only works on Filters. Returns a filter with the reversed
 * time intervals as the incoming filter.
 *
 * @param a TimeSplitterType filter.
 */
TimeSplitterType operator ~(const TimeSplitterType& a)
{
  TimeSplitterType out, temp;
  //First, you must remove any overlapping intervals, otherwise the output is stupid.
  temp = removeFilterOverlap(a);

  //No entries: then make a "filter" that keeps everything
  if ((temp.size()==0) )
  {
    out.push_back( SplittingInterval(DateAndTime::minimum(), DateAndTime::maximum(), 0 ) );
    return out;
  }

  TimeSplitterType::const_iterator ait;
  ait=temp.begin();
  if (ait != temp.end())
  {
    //First entry; start at -infinite time
    out.push_back( SplittingInterval( DateAndTime::minimum(), ait->start(), 0) );
    //Now start at the second entry
    while (ait != temp.end())
    {
      DateAndTime start, stop;
      start = ait->stop();
      ait++;
      if (ait==temp.end())
      { //Reached the end - go to inf
        stop = DateAndTime::maximum();
      }
      else
      { //Stop at the start of the next entry
        stop = ait->start();
      }
      out.push_back( SplittingInterval(start, stop, 0) );
    }
  }
  return out;
}

}
}
