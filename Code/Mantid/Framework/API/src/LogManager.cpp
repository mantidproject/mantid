//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LogManager.h"
#include "MantidAPI/PropertyNexus.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>

namespace Mantid
{
namespace API
{

using namespace Kernel;

  namespace
  {
    /// static logger
    Logger g_log("LogManager");
  }

 /// Name of the log entry containing the proton charge when retrieved using getProtonCharge
 const char * LogManager::PROTON_CHARGE_LOG_NAME = "gd_prtn_chrg";

  //----------------------------------------------------------------------
  // Public member functions
  //----------------------------------------------------------------------
  /**
   * Default constructor
   */
  LogManager::LogManager() : m_manager(), m_singleValueCache()
  {
  }

  /**
   * Destructor
   */
  LogManager::~LogManager()
  {
  }

  /**
   * Copy constructor
   * @param copy :: The object to initialize the copy from
   */
  LogManager::LogManager(const LogManager& copy) : m_manager(copy.m_manager),
    m_singleValueCache(copy.m_singleValueCache)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Assignment operator
   * @param rhs :: The object whose properties should be copied into this
   * @returns A cont reference to the copied object
   */
  const LogManager& LogManager::operator=(const LogManager& rhs)
  {
    if( this == &rhs ) return *this;
    m_manager = rhs.m_manager; 
    return *this;
  }


  /**
  * Set the run start and end
  * @param start :: The run start
  * @param end :: The run end
  */
  void LogManager::setStartAndEndTime(const Kernel::DateAndTime & start, const Kernel::DateAndTime & end)
  {
    this->addProperty<std::string>("start_time", start.toISO8601String(), true);
    this->addProperty<std::string>("end_time", end.toISO8601String(), true);
  }

  /** Return the run start time as given by the 'start_time' or 'run_start' property.
   *  'start_time' is tried first, falling back to 'run_start' if the former isn't found.
   *  @returns The start time of the run
   *  @throws std::runtime_error if neither property is defined
   */
  const Kernel::DateAndTime LogManager::startTime() const
  {
    const std::string start_prop("start_time");
    if ( hasProperty(start_prop) )
    {
      try {
        DateAndTime start_time(getProperty(start_prop)->value());
        if (start_time != DateAndTimeHelpers::GPS_EPOCH)
        {
          return start_time;
        }
      } catch (std::invalid_argument&) { /*Swallow and move on*/ }
    }

    const std::string run_start_prop("run_start");
    if ( hasProperty(run_start_prop) )
    {
      try {
        DateAndTime start_time(getProperty(run_start_prop)->value());
        if (start_time != DateAndTimeHelpers::GPS_EPOCH)
        {
          return start_time;
        }
      } catch (std::invalid_argument&) { /*Swallow and move on*/ }
    }

    throw std::runtime_error("No valid start time has been set for this run.");
  }

  /** Return the run end time as given by the 'end_time' or 'run_end' property.
   *  'end_time' is tried first, falling back to 'run_end' if the former isn't found.
   *  @returns The end time of the run
   *  @throws std::runtime_error if neither property is defined
   */
  const Kernel::DateAndTime LogManager::endTime() const
  {
    const std::string end_prop("end_time");
    if ( hasProperty(end_prop) )
    {
      try {
        return DateAndTime(getProperty(end_prop)->value());
      } catch (std::invalid_argument&) { /*Swallow and move on*/ }
    }

    const std::string run_end_prop("run_end");
    if (hasProperty(run_end_prop))
    {
      try {
        return DateAndTime(getProperty(run_end_prop)->value());
      } catch (std::invalid_argument&) { /*Swallow and move on*/ }
    }

    throw std::runtime_error("No valid end time has been set for this run.");
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
   *  absolute time range.
   *
   * @param start :: Absolute start time. Any log entries at times >= to this time are kept.
   * @param stop :: Absolute stop time. Any log entries at times < than this time are kept.
   */
  void LogManager::filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop)
  {
    //The propery manager operator will make all timeseriesproperties filter.
    m_manager.filterByTime(start, stop);

  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Split a run by time (splits the TimeSeriesProperties contained).
   *
   *
   * @param splitter :: TimeSplitterType with the intervals and destinations.
   * @param outputs :: Vector of output runs.
   */
  void LogManager::splitByTime(TimeSplitterType& splitter, std::vector< LogManager * > outputs) const
  {
    //Make a vector of managers for the splitter. Fun!
    const size_t n = outputs.size();
    std::vector< PropertyManager *> output_managers(outputs.size(),NULL);
    for (size_t i=0; i<n; i++)
    {
      if (outputs[i])
      {
        output_managers[i] = &(outputs[i]->m_manager);
      }
    }

    //Now that will do the split down here.
    m_manager.splitByTime(splitter, output_managers);

  }

  //-----------------------------------------------------------------------------------------------
  /**
   * Filter the run by the given boolean log. It replaces all time
   * series properties with filtered time series properties
   * @param filter :: A boolean time series to filter each log on
   */
  void LogManager::filterByLog(const Kernel::TimeSeriesProperty<bool> & filter)
  {
    // This will invalidate the cache
    m_singleValueCache.clear();
    m_manager.filterByProperty(filter);
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Add data to the object in the form of a property
   * @param prop :: A pointer to a property whose ownership is transferred to this object
   * @param overwrite :: If true, a current value is overwritten. (Default: False)
   */
  void LogManager::addProperty(Kernel::Property *prop, bool overwrite)
  {
    // Make an exception for the proton charge
    // and overwrite it's value as we don't want to store the proton charge in two separate locations
    // Similar we don't want more than one run_title
    std::string name = prop->name();
    if( hasProperty(name) && (overwrite || prop->name() == PROTON_CHARGE_LOG_NAME || prop->name()=="run_title") )
    {
      removeProperty(name);
    }
    m_manager.declareProperty(prop, "");
  }

  //-----------------------------------------------------------------------------------------------
  /**
   * Returns true if the named property exists
   * @param name :: The name of the property
   * @return True if the property exists, false otherwise
   */
  bool LogManager::hasProperty(const std::string & name) const
  {
    return m_manager.existsProperty(name);
  }

  //-----------------------------------------------------------------------------------------------
  /**
   * Remove a named property
   * @param name :: The name of the property
   * @param delProperty :: If true the property is deleted (default=true)
   * @return True if the property exists, false otherwise
   */

  void LogManager::removeProperty(const std::string &name, bool delProperty)
  {
    // Remove any cached entries for this log. Need to make this more general
    for(unsigned int stat = 0; stat < 7; ++stat)
    {
      m_singleValueCache.removeCache(std::make_pair(name,(Math::StatisticType)stat));
    }
    m_manager.removeProperty(name, delProperty);
  }


  //-----------------------------------------------------------------------------------------------
  /** Return the total memory used by the run object, in bytes.
   */
  size_t LogManager::getMemorySize() const
  {
    size_t total = 0;
    std::vector< Property*> props = m_manager.getProperties();
    for (size_t i=0; i < props.size(); i++)
    {
      Property * p = props[i];
      if (p)
        total += p->getMemorySize() + sizeof(Property *);
    }
    return total;
  }

  /**
   * Returns a property as a time series property. It will throw if it is not valid or the
   * property does not exist
   * @param name The name of a time-series property
   * @return A pointer to the time-series property
   */
  template<typename T>
  Kernel::TimeSeriesProperty<T> * LogManager::getTimeSeriesProperty(const std::string & name) const
  {
    Kernel::Property *prop = getProperty(name);
    if(Kernel::TimeSeriesProperty<T>* tsp = dynamic_cast<Kernel::TimeSeriesProperty<T>*>(prop))
    {
      return tsp;
    }
    else
    {
      throw std::invalid_argument("Run::getTimeSeriesProperty - '" + name + "' is not a TimeSeriesProperty");
    }
  }

  /**
   * Get the value of a property as the requested type. Throws if the type is not correct
   * @param name :: The name of the property
   * @return The value of as the requested type
   */
  template<typename HeldType>
  HeldType LogManager::getPropertyValueAsType(const std::string & name) const
  {
    Kernel::Property *prop = getProperty(name);
    if(Kernel::PropertyWithValue<HeldType>* valueProp = dynamic_cast<Kernel::PropertyWithValue<HeldType>*>(prop))
    {
      return (*valueProp)();
    }
    else
    {
      throw std::invalid_argument("Run::getPropertyValueAsType - '" + name + "' is not of the requested type");
    }
  }

  /**
   * Returns a property as a single double value from its name @see getPropertyAsSingleValue
   * @param name :: The name of the property
   * @param statistic :: The statistic to use to calculate the single value (default=Mean) @see StatisticType
   * @return A single double value
   */
  double LogManager::getPropertyAsSingleValue(const std::string & name, Kernel::Math::StatisticType statistic) const
  {
    double singleValue(0.0);
    const auto key = std::make_pair(name, statistic);
    if(!m_singleValueCache.getCache(key, singleValue))
    {
      const Property *log = getProperty(name);
      if(auto singleDouble = dynamic_cast<const PropertyWithValue<double>*>(log))
      {
        singleValue  = (*singleDouble)();
      }
      else if(auto seriesDouble = dynamic_cast<const TimeSeriesProperty<double>*>(log))
      {
        singleValue = Mantid::Kernel::filterByStatistic(seriesDouble, statistic);
      }
      else
      {
        throw std::invalid_argument("Run::getPropertyAsSingleValue - Property \"" + name + "\" is not a single double or time series double.");
      }
      // Put it in the cache
      m_singleValueCache.setCache(key, singleValue);
    }
    return singleValue;
  }

  /**
   * Get a pointer to a property by name
   * @param name :: The name of a property, throws an Exception::NotFoundError if it does not exist
   * @return A pointer to the named property
   */
  Kernel::Property * LogManager::getProperty(const std::string & name) const
  {
    return m_manager.getProperty(name);
  }

  /** Clear out the contents of all logs of type TimeSeriesProperty.
   *  Single-value properties will be left unchanged.
   *
   *  The method has been fully implemented here instead of as a pass-through to
   *  PropertyManager to limit its visibility to Run clients.
   */
  void LogManager::clearTimeSeriesLogs()
  {
    auto & props = getProperties();

    // Loop over the set of properties, identifying those that are time-series properties
    // and then clearing them out.
    for ( auto it = props.begin(); it != props.end(); ++it)
    {
      if ( auto tsp = dynamic_cast<ITimeSeriesProperty*>(*it) )
      {
        tsp->clear();
      }
    }
  }

  /** Clears out all but the last entry of all logs of type TimeSeriesProperty
   *  Check the documentation/definition of TimeSeriesProperty::clearOutdated for
   *  the definition of 'last entry'.
   */
  void LogManager::clearOutdatedTimeSeriesLogValues()
  {
    auto & props = getProperties();
    for ( auto it = props.begin(); it != props.end(); ++it)
    {
      if ( auto tsp = dynamic_cast<ITimeSeriesProperty*>(*it) )
      {
        tsp->clearOutdated();
      }
    }
  }

  //--------------------------------------------------------------------------------------------
  /** Save the object to an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to create
   * @param keepOpen :: do not close group on exit to allow overloading and child classes writing to the same group 
   */
  void LogManager::saveNexus(::NeXus::File * file, const std::string & group,bool keepOpen) const
  {
    file->makeGroup(group, "NXgroup", 1);
    file->putAttr("version", 1);
       
    // Save all the properties as NXlog
    std::vector<Property *> props = m_manager.getProperties();
    for (size_t i=0; i<props.size(); i++)
    {
      try
      {
        PropertyNexus::saveProperty(file, props[i]);
      }
      catch(std::invalid_argument &exc)
      {
        g_log.warning(exc.what());
      }
    }
    if(!keepOpen)file->closeGroup();
  }

  //--------------------------------------------------------------------------------------------
  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to open. Empty string to NOT open a group, but
   * @param keepOpen :: do not close group on exit to allow overloading and child classes reading from the same group
   * load any NXlog in the current open group.
   */
  void LogManager::loadNexus(::NeXus::File * file, const std::string & group,bool keepOpen)
  {
    if (!group.empty()) file->openGroup(group, "NXgroup");

    std::map<std::string, std::string> entries;
    file->getEntries(entries);
    std::map<std::string, std::string>::iterator it = entries.begin();
    std::map<std::string, std::string>::iterator it_end = entries.end();
    for (; it != it_end; ++it)
    {
      // Get the name/class pair
      const std::pair<std::string, std::string> & name_class = *it;
      // NXLog types are the main one.
      if (name_class.second == "NXlog")
      {
        Property * prop = PropertyNexus::loadProperty(file, name_class.first);
        if (prop)
        {
          if (m_manager.existsProperty(prop->name() ))
          {
            m_manager.removeProperty(prop->name() );
          }
          m_manager.declareProperty(prop);
        }
      }
    }
    if (!(group.empty()||keepOpen))file->closeGroup();
  }

  /**
   * Clear the logs.
   */
  void LogManager::clearLogs()
  {
    m_manager.clear();
  }

  //-----------------------------------------------------------------------------------------------------------------------
  // Private methods
  //-----------------------------------------------------------------------------------------------------------------------
 


  /** @cond */
  /// Macro to instantiate concrete template members
#define INSTANTIATE(TYPE) \
  template MANTID_API_DLL Kernel::TimeSeriesProperty<TYPE> * LogManager::getTimeSeriesProperty(const std::string &) const;\
  template MANTID_API_DLL TYPE LogManager::getPropertyValueAsType(const std::string &) const;

  INSTANTIATE(double);
  INSTANTIATE(int);
  INSTANTIATE(long);
  INSTANTIATE(uint32_t);
  INSTANTIATE(uint64_t);
  INSTANTIATE(std::string);
  INSTANTIATE(bool);

  template MANTID_API_DLL uint16_t LogManager::getPropertyValueAsType(const std::string &) const;
  template MANTID_API_DLL std::vector<double> LogManager::getPropertyValueAsType(const std::string &) const;
  template MANTID_API_DLL std::vector<size_t> LogManager::getPropertyValueAsType(const std::string &) const;
  template MANTID_API_DLL std::vector<int> LogManager::getPropertyValueAsType(const std::string &) const;
  template MANTID_API_DLL std::vector<long> LogManager::getPropertyValueAsType(const std::string &) const;
  /** @endcond */

} //API namespace

}

