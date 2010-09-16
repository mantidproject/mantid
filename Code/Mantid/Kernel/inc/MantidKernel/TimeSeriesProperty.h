#ifndef MANTID_KERNEL_TIMESERIESPROPERTY_H_
#define MANTID_KERNEL_TIMESERIESPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <cctype>
#include "boost/date_time/posix_time/posix_time.hpp"

namespace Mantid
{
namespace Kernel
{

/** 
 A specialised Property class for holding a series of time-value pairs.
 Required by the LoadLog class.
 
 @author Russell Taylor, Tessella Support Services plc
 @date 26/11/2007
 @author Anders Markvardsen, ISIS, RAL
 @date 12/12/2007

 Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template<typename TYPE>
class DLLExport TimeSeriesProperty: public Property
{
private:
  /// typedef for the storage of a TimeSeries
  typedef std::multimap<dateAndTime, TYPE> timeMap;
  /// Holds the time series data 
  timeMap m_propertySeries;

  /// The number of values (or time intervals) in the time series. It can be different from m_propertySeries.size()
  int m_size;

public:
  /** Constructor
   *  @param name The name to assign to the property
   */
  explicit TimeSeriesProperty(const std::string &name) :
    Property(name, typeid(timeMap)), m_propertySeries(), m_size()
  {
  }

  /// Virtual destructor
  virtual ~TimeSeriesProperty()
  {
  }

  /// 'Virtual copy constructor'
  Property* clone() { return new TimeSeriesProperty<TYPE>(*this); }


  //--------------------------------------------------------------------------------------
  ///Add the value of another property
  virtual TimeSeriesProperty& operator+=( Property * right )
  {
    TimeSeriesProperty * rhs = dynamic_cast< TimeSeriesProperty * >(right);

    //Concatenate the maps!
    m_propertySeries.insert(rhs->m_propertySeries.begin(), rhs->m_propertySeries.end());

    this->countSize();

    return *this;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
   *  absolute time range.
   *
   * @param start Absolute start time. Any log entries at times >= to this time are kept.
   * @param stop Absolute stop time. Any log entries at times < than this time are kept.
   */
  void filterByTime(const Kernel::dateAndTime start, const Kernel::dateAndTime stop)
  {
    typename timeMap::iterator it;
    for (it = m_propertySeries.begin(); it != m_propertySeries.end(); it++)
    {
      dateAndTime t = it->first;
      if ((t < start) || (t >= stop))
      {
        //Is outside the range we are keeping, so erase that.
        m_propertySeries.erase(it);
      }
    }
    //The function below is stupid. Should be removed or fixed.
    this->countSize();
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Split out a time series property by time intervals.
   *
   * @param splitter a TimeSplitterType object containing the list of intervals and destinations.
   * @param outputs A vector of output TimeSeriesProperty pointers of the same type.
   */
  void splitByTime(TimeSplitterType& splitter, std::vector< TimeSeriesProperty<TYPE> * > outputs)
  {
    int numOutputs = static_cast<int>( outputs.size() );
    if (numOutputs <= 0)
      return;

    //Clear the outputs before you start
    for (int i=0; i < numOutputs; i++)
    {
      TimeSeriesProperty<TYPE> * myOutput = outputs[i];
      if (myOutput)
        myOutput->m_propertySeries.clear();
    }


    //We will be iterating through all the entries in the the map
    typename timeMap::iterator it;
    it = m_propertySeries.begin();

    //And at the same time, iterate through the splitter
    Kernel::TimeSplitterType::iterator itspl = splitter.begin();

    //Info of each splitter
    dateAndTime start, stop;
    int index;

    while (itspl != splitter.end())
    {
      //Get the splitting interval times and destination
      start = itspl->startDate();
      stop = itspl->stopDate();
      index = itspl->index();

      //Skip the events before the start of the time
      while ((it != this->m_propertySeries.end()) && (it->first < start))
        it++;

      //Go through all the events that are in the interval (if any)
      while ((it != this->m_propertySeries.end()) && (it->first < stop))
      {
        if ((index >= 0) && (index < numOutputs))
        {
          TimeSeriesProperty<TYPE> * myOutput = outputs[index];
          //Copy the log out to the output
          if (myOutput)
            myOutput->addValue(it->first, it->second);
        }
        ++it;
      }

      //Go to the next interval
      itspl++;
      //But if we reached the end, then we are done.
      if (itspl==splitter.end())
        break;

      //No need to keep looping through the filter if we are out of events
      if (it == this->m_propertySeries.end())
        break;

    } //Looping through entries in the splitter vector


    //Count the sizes (this function is still stupid and slow and should not be necessary, but I don't know if I should take it out)
    for (int i=0; i < numOutputs; i++)
    {
      TimeSeriesProperty<TYPE> * myOutput = outputs[index];
      if (myOutput)
        myOutput->countSize();
    }

  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Fill a TimeSplitterType that will filter the events by matching
   * log values >= min and < max.
   *
   * @param split Splitter that will be filled.
   * @param min min value
   * @param max max value
   * @param TimeTolerance offset added to times in seconds.
   */
  void makeFilterByValue(TimeSplitterType& split, TYPE min, TYPE max, double TimeTolerance)
  {
    //Do nothing if the log is empty.
    if (m_propertySeries.size() == 0)
      return;

    typename timeMap::iterator it;
    bool lastGood = false;
    bool isGood;
    time_duration tol = DateAndTime::duration_from_seconds( TimeTolerance );
    int numgood = 0;
    dateAndTime lastTime, t;
    PulseTimeType start, stop;

    for (it = m_propertySeries.begin(); it != m_propertySeries.end(); it++)
    {
      lastTime = t;
      //The new entry
      t = it->first;
      TYPE val = it->second;

      //A good value?
      isGood = ((val >= min) && (val < max));
      if (isGood)
        numgood++;

      if (isGood != lastGood)
      {
        //We switched from bad to good or good to bad

        if (isGood)
        {
          //Start of a good section
          start = DateAndTime::get_from_absolute_time( t - tol );
        }
        else
        {
          //End of the good section
          if (numgood == 1)
          {
            //There was only one point with the value. Use the last time, + the tolerance, as the end time
            stop = DateAndTime::get_from_absolute_time( lastTime + tol );
            split.push_back( SplittingInterval(start, stop, 0) );
          }
          else
          {
            //At least 2 good values. Save the end time
            stop = DateAndTime::get_from_absolute_time( t + tol );
            split.push_back( SplittingInterval(start, stop, 0) );
          }
          //Reset the number of good ones, for next time
          numgood = 0;
        }
        lastGood = isGood;
      }
    }

    if (numgood > 0)
    {
      //The log ended on "good" so we need to close it using the last time we found
      stop = DateAndTime::get_from_absolute_time( t + tol );
      split.push_back( SplittingInterval(start, stop, 0) );
      numgood = 0;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /* Get the time series property as a string of 'time  value'
   *
   * @return time series property as a string
   */
  std::string value() const
  {
    std::stringstream ins;

    typename timeMap::const_iterator p = m_propertySeries.begin();

    while (p != m_propertySeries.end())
    {
      try
      {
        ins << p->first << "  " << p->second << std::endl;
      }
      catch (...)
      {
        //Some kind of error; for example, invalid year, can occur when converying boost time.
        ins << "Error Error" << std::endl;
      }
      p++;
    }

    return ins.str();
  }

  //-----------------------------------------------------------------------------------------------
  /**  New method to return time series value pairs as std::vector<std::string>
   *
   * @return time series property values as a string vector "<time_t> value"
   */
  std::vector<std::string> time_tValue() const
  {
    std::vector<std::string> values;
    values.reserve(m_propertySeries.size());

    typename timeMap::const_iterator p = m_propertySeries.begin();

    while (p != m_propertySeries.end())
    {
      std::stringstream line;
      line << p->first << " " << p->second;
      values.push_back(line.str());
      p++;
    }

    return values;
  }

  //-----------------------------------------------------------------------------------------------
  /**  Return the time series as a C++ map<dateAndTime, TYPE>
   *
   * @return time series property values as map
   */
  std::map<dateAndTime, TYPE> valueAsMap() const
  {
    std::map<dateAndTime, TYPE> asMap;
    if (m_propertySeries.size() == 0)
      return asMap;
    typename timeMap::const_iterator p = m_propertySeries.begin();
    TYPE d = p->second;
    for (; p != m_propertySeries.end(); p++)
    {
      //Skips any entries where the value was unchanged.
      if (p != m_propertySeries.begin() && p->second == d) continue;
      d = p->second;
      asMap[p->first] = d;
    }

    return asMap;
  }



  //-----------------------------------------------------------------------------------------------
  /** Not implemented in this class
   *  @throws Exception::NotImplementedError Not yet implemented
   * @return Nothing in this case
   */
  std::string setValue(const std::string&)
  {
    throw Exception::NotImplementedError("Not yet");
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a value to the map
   *  @param time The time as a boost::posix_time::ptime value
   *  @param value The associated value
   *  @return True if insertion successful (i.e. identical time not already in map
   */
  bool addValue(const boost::posix_time::ptime &time, const TYPE value)
  {
    m_size++;
    return m_propertySeries.insert(typename timeMap::value_type(time, value)) != m_propertySeries.end();
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a value to the map
   *  @param time The time as a string in the format: (ISO 8601) yyyy-mm-ddThh:mm:ss
   *  @param value The associated value
   *  @return True if insertion successful (i.e. identical time not already in map
   */
  bool addValue(const std::string &time, const TYPE value)
  {
    return addValue(Kernel::DateAndTime::create_DateAndTime_FromISO8601_String(time), value);
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a value to the map
   *  @param time The time as a time_t value
   *  @param value The associated value
   *  @return True if insertion successful (i.e. identical time not already in map
   */
  bool addValue(const std::time_t &time, const TYPE value)
  {
    return addValue(boost::posix_time::from_time_t(time), value);
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the value at a particular time
   *  @param t time
   *  @return Value at time \a t
   */
  TYPE getSingleValue(const dateAndTime& t) const
  {
    typename timeMap::const_reverse_iterator it = m_propertySeries.rbegin();
    for (; it != m_propertySeries.rend(); it++)
      if (it->first <= t)
        return it->second;
    if (m_propertySeries.size() == 0)
      return TYPE();
    else
      return m_propertySeries.begin()->second;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns total value, added up for all times
   *  @param t time
   *  @return Value at time \a t
   */
  TYPE getTotalValue() const
  {
    typename timeMap::const_iterator it = m_propertySeries.begin();
    TYPE total = 0;
    for (; it != m_propertySeries.end(); it++)
      total += it->second;
    return total;
  }

  //-----------------------------------------------------------------------------------------------
  /// Returns the number of values at UNIQUE time intervals in the time series
  int size() const
  {
    return m_size;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the real size of the time series property map:
   * the number of entries, including repeated ones.
   */
  int realSize() const
  {
    return m_propertySeries.size();
  }
  //-----------------------------------------------------------------------------------------------
  /** Returns n-th value in an incredibly inefficient way.
   *  @param n index
   *  @return Value 
   */
  TYPE nthValue(int n) const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");

    typename timeMap::const_iterator it = m_propertySeries.begin();
    for (int j = 0; it != m_propertySeries.end(); it++)
    {
      if (m_propertySeries.count(it->first) > 1)
        continue;
      if (j == n)
        return it->second;
      j++;
    }

    return m_propertySeries.rbegin()->second;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the last value
   *  @return Value 
   */
  TYPE lastValue() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.rbegin()->second;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the last time
   *  @return Value 
   */
  dateAndTime lastTime() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.rbegin()->first;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the first value
   *  @return Value 
   */
  TYPE firstValue() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.begin()->second;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the first time
   *  @return Value 
   */
  dateAndTime firstTime() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.begin()->first;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns n-th valid time interval, in a very inefficient way.
   *  @param n index
   *  @return n-th time interval
   */
  TimeInterval nthInterval(int n) const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");

    typename timeMap::const_iterator it = m_propertySeries.begin();
    dateAndTime t = it->first;
    for (int j = 0; it != m_propertySeries.end(); it++)
    {
      if (m_propertySeries.count(it->first) > 1)
        continue;
      if (j == n)
      {
        typename timeMap::const_iterator it1 = it;
        it1++;
        if (it1 != m_propertySeries.end())
          return TimeInterval(it->first, it1->first);
        else
        {
          //We are at the end of the series
          // ---> Previous functionality = use a d = 1/10th of the total time. Doesn't make sense to me!
          // ----> time_duration d = (m_propertySeries.rbegin()->first - m_propertySeries.begin()->first) / 10;

          //Use the previous interval instead
          typename timeMap::const_iterator it2 = it;
          it2--;
          time_duration d = it->first - it2->first;

          //Make up an end time.
          dateAndTime endTime = it->first + d;
          return TimeInterval(it->first, endTime);
        }
        if (it1 != m_propertySeries.end() && it1->first == it->first)
          continue;
      }
      t = it->first;
      j++;
    }

    return TimeInterval();
  }


  //-----------------------------------------------------------------------------------------------
  /** Divide the property into  allowed and disallowed time intervals according to \a filter.
   Repeated time-value pairs (two same time and value entries) mark the start of a gap in the values. 
   The gap ends and an allowed time interval starts when a single time-value is met.
   @param filter The filter mask to apply
   */
  void filterWith(const TimeSeriesProperty<bool>* filter)
  {
    std::map<dateAndTime, bool> fmap = filter->valueAsMap();
    std::map<dateAndTime, bool>::const_iterator f = fmap.begin();
	if(fmap.empty()) return;
    typename timeMap::iterator it = m_propertySeries.begin();
    if (f->first < it->first)// expand this series
    {
      m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
      it = m_propertySeries.begin();
    }
    bool hide = !f->second;
    bool finished = false;
    // At the start f->first >= it->first
    for (; f != fmap.end(); hide = !(f++)->second)
    {
      // element next to it
      typename timeMap::iterator it1 = it;
      it1++;
      if (it1 == m_propertySeries.end())
      {
        if (f->first < it->first && hide)
          if (m_propertySeries.count(it->first) == 1)
            m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
        finished = true;
        break;
      }
      else
        // we want f to be between it and it1
        while (f->first < it->first || f->first >= it1->first)
        {
          // we are still in the scope of the previous filter value, and if it 'false' we must hide the interval
          if (hide)
          {
            if (m_propertySeries.count(it->first) == 1)
            {
              it = m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
            }
            if (it1 != m_propertySeries.end() && m_propertySeries.count(it1->first) == 1 && f->first
                > it1->first)
              m_propertySeries.insert(typename timeMap::value_type(it1->first, it1->second));
          }
          it++;
          it1 = it;
          it1++;
          if (it1 == m_propertySeries.end())
          {
            if (hide && f->first != it->first)
            {
              if (m_propertySeries.count(it->first) == 1)
                it = m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
              it1 = m_propertySeries.end();
            }
            finished = true;
            break;
          }
        };
      bool gap = m_propertySeries.count(it->first) > 1;
      if (gap && it1 != m_propertySeries.end())
      {
        if (f->second == true)
          it = m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
      }
      else
      {
        if (f->second == false)
        {
          if (f->first != it->first)
            m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
          it = m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
        }
      }
    }
    // If filter stops in the middle of this series with value 'false' (meaning hide the rest of the values)
    if (!finished && fmap.rbegin()->second == false)
    {
      for (; it != m_propertySeries.end(); it++)
        if (m_propertySeries.count(it->first) == 1)
          it = m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
    }

    // Extend this series if the filter end later than the data
    if (finished && f != fmap.end())
    {
      TYPE v = m_propertySeries.rbegin()->second;
      for (; f != fmap.end(); f++)
      {
        m_propertySeries.insert(typename timeMap::value_type(f->first, v));
        if (!f->second)
          m_propertySeries.insert(typename timeMap::value_type(f->first, v));
      }
    }

    countSize();
  }


  //-----------------------------------------------------------------------------------------------
  /// Restores the property to the unsorted state
  void clearFilter()
  {
    std::map<dateAndTime, TYPE> pmap = valueAsMap();
    m_propertySeries.clear();
    m_size = 0;
    if (pmap.size() == 0)
      return;
    TYPE val = pmap.begin()->second;
    typename std::map<dateAndTime, TYPE>::const_iterator it = pmap.begin();
    addValue(it->first, it->second);
    for (; it != pmap.end(); it++)
    {
      if (it->second != val)
        addValue(it->first, it->second);
      val = it->second;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /// Clones the property
  TimeSeriesProperty<TYPE>* clone() const
  {
    TimeSeriesProperty<TYPE>* p = new TimeSeriesProperty<TYPE> (name());
    p->m_propertySeries = m_propertySeries;
    p->m_size = m_size;
    return p;
  }


  //-----------------------------------------------------------------------------------------------
  /** Updates m_size.
   * TODO: Warning! COULD BE VERY SLOW, since it counts each entry each time.
   */
  void countSize()
  {
    m_size = 0;
    if (m_propertySeries.size() == 0)
      return;
    typename timeMap::const_iterator it = m_propertySeries.begin();
    for (; it != m_propertySeries.end(); it++)
    {
      if (m_propertySeries.count(it->first) == 1)
        m_size++;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /**  Check if str has the right time format 
   *   @param str The string to check
   *   @return True if the format is correct, false otherwise.
   */
  static bool isTimeString(const std::string &str)
  {
      if (str.size() < 19) return false;
      if (!isdigit(str[0])) return false;
      if (!isdigit(str[1])) return false;
      if (!isdigit(str[2])) return false;
      if (!isdigit(str[3])) return false;
      if (!isdigit(str[5])) return false;
      if (!isdigit(str[6])) return false;
      if (!isdigit(str[8])) return false;
      if (!isdigit(str[9])) return false;
      if (!isdigit(str[11])) return false;
      if (!isdigit(str[12])) return false;
      if (!isdigit(str[14])) return false;
      if (!isdigit(str[15])) return false;
      if (!isdigit(str[17])) return false;
      if (!isdigit(str[18])) return false;
      return true;
  }
  
  //-----------------------------------------------------------------------------------------------
  /** This doesn't check anything -we assume these are always valid
   * 
   *  @returns an empty string ""
   */
  std::string isValid() const { return ""; }


  //-----------------------------------------------------------------------------------------------
  /* Not implemented in this class
   * @throws Exception::NotImplementedError Not yet implemented
   */
  std::string getDefault() const
  {
    throw Exception::NotImplementedError("TimeSeries properties don't have defaults");
  }

  //-----------------------------------------------------------------------------------------------
  ///Not used in this class and always returns false
  bool isDefault() const { return false; }

  /// static reference to the logger class
  //static Kernel::Logger& g_log;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_TIMESERIESPROPERTY_H_*/
