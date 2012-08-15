//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Run.h"
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
  /// The number of log entries summed when adding a run
  const int ADDABLES = 6;
  /// The names of the log entries summed when adding two runs together
  const std::string ADDABLE[ADDABLES] = {"tot_prtn_chrg", "rawfrm", "goodfrm", "dur", "gd_prtn_chrg", "uA.hour"};
  /// Name of the log entry containing the proton charge when retrieved using getProtonCharge
  const char * PROTON_CHARGE_LOG_NAME = "gd_prtn_chrg";
  /// Name of the goniometer log when saved to a NeXus file
  const char * GONIOMETER_LOG_NAME = "goniometer";
  /// Name of the stored histogram bins log when saved to NeXus
  const char * HISTO_BINS_LOG_NAME = "processed_histogram_bins";
}
// Get a reference to the logger
Kernel::Logger& Run::g_log = Kernel::Logger::get("Run");

  //----------------------------------------------------------------------
  // Public member functions
  //----------------------------------------------------------------------
  /**
   * Default constructor
   */
  Run::Run() : m_manager(), m_goniometer(), m_histoBins(), m_singleValueCache()
  {
  }

  /**
   * Destructor
   */
  Run::~Run()
  {
  }

  /**
   * Copy constructor
   * @param copy :: The object to initialize the copy from
   */
  Run::Run(const Run& copy) : m_manager(copy.m_manager),
    m_goniometer(copy.m_goniometer), m_singleValueCache(copy.m_singleValueCache)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Assignment operator
   * @param rhs :: The object whose properties should be copied into this
   * @returns A cont reference to the copied object
   */
  const Run& Run::operator=(const Run& rhs)
  {
    if( this == &rhs ) return *this;
    m_manager = rhs.m_manager;
    m_goniometer = rhs.m_goniometer;
    return *this;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Adds just the properties that are safe to add. All time series are
   * merged together and the list of addable properties are added
   * @param rhs The object that is being added to this.
   * @returns A reference to the summed object
   */
  Run& Run::operator+=(const Run& rhs)
  {
    //merge and copy properties where there is no risk of corrupting data
    mergeMergables(m_manager, rhs.m_manager);

    // Other properties are added to gether if they are on the approved list
    for(int i = 0; i < ADDABLES; ++i )
    {
      if (rhs.m_manager.existsProperty(ADDABLE[i]))
      {
        // get a pointer to the property on the right-handside workspace
        Property * right = rhs.m_manager.getProperty(ADDABLE[i]);

        // now deal with the left-handside
        if (m_manager.existsProperty(ADDABLE[i]))
        {
          Property * left = m_manager.getProperty(ADDABLE[i]);
          left->operator+=(right);
        }
        else
          //no property on the left-handside, create one and copy the right-handside across verbatum
          m_manager.declareProperty(right->clone(), "");
      }
    }
    return *this;
  }

  /**
  * Set the run start and end
  * @param start :: The run start
  * @param end :: The run end
  */
  void Run::setStartAndEndTime(const Kernel::DateAndTime & start, const Kernel::DateAndTime & end)
  {
    this->addProperty<std::string>("start_time", start.toISO8601String(), true);
    this->addProperty<std::string>("end_time", end.toISO8601String(), true);
  }

  /// Return the run start time
  const Kernel::DateAndTime Run::startTime() const
  {
	// Use start_time if found, else use run_start
    const std::string start_prop("start_time");
	const std::string run_start_prop("run_start");
    if( this->hasProperty(start_prop)  ) 
    {
      std::string start = this->getProperty(start_prop)->value();
      return DateAndTime(start);
    }  
	else if (  this->hasProperty(run_start_prop) ) 
	{
	 std::string start = this->getProperty(run_start_prop)->value();
      return DateAndTime(start);

	}
    else
    {
      throw std::runtime_error("Run::startTime() - No start time has been set for this run.");
    }
  }

  /// Return the run end time
  const Kernel::DateAndTime Run::endTime() const
  {
	// Use end_time if found, else use run_end
    const std::string end_prop("end_time");
	const std::string run_end_prop("run_end");
    if( this->hasProperty(end_prop) )
    {
      std::string end = this->getProperty(end_prop)->value();
      return DateAndTime(end);
    }
    else if( this->hasProperty(run_end_prop) )
    {
      std::string end = this->getProperty(run_end_prop)->value();
      return DateAndTime(end);
    }
    {
      throw std::runtime_error("Run::endTime() - No end time has been set for this run.");
    }
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
   *  absolute time range.
   *
   * Total proton charge will get re-integrated after filtering.
   *
   * @param start :: Absolute start time. Any log entries at times >= to this time are kept.
   * @param stop :: Absolute stop time. Any log entries at times < than this time are kept.
   */
  void Run::filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop)
  {
    //The propery manager operator will make all timeseriesproperties filter.
    m_manager.filterByTime(start, stop);

    //Re-integrate proton charge
    this->integrateProtonCharge();
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Split a run by time (splits the TimeSeriesProperties contained).
   *
   * Total proton charge will get re-integrated after filtering.
   *
   * @param splitter :: TimeSplitterType with the intervals and destinations.
   * @param outputs :: Vector of output runs.
   */
  void Run::splitByTime(TimeSplitterType& splitter, std::vector< Run * > outputs) const
  {
    //Make a vector of managers for the splitter. Fun!
    std::vector< PropertyManager *> output_managers;
    size_t n = outputs.size();
    for (size_t i=0; i<n; i++)
    {
      if (outputs[i])
        output_managers.push_back( &(outputs[i]->m_manager) );
      else
        output_managers.push_back( NULL );
    }

    //Now that will do the split down here.
    m_manager.splitByTime(splitter, output_managers);

    //Re-integrate proton charge of all outputs
    for (size_t i=0; i<n; i++)
    {
      if (outputs[i])
        outputs[i]->integrateProtonCharge();
    }
  }

  //-----------------------------------------------------------------------------------------------
  /**
   * Filter the run by the given boolean log. It replaces all time
   * series properties with filtered time series properties
   * @param filter :: A boolean time series to filter each log on
   */
  void Run::filterByLog(const Kernel::TimeSeriesProperty<bool> & filter)
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
  void Run::addProperty(Kernel::Property *prop, bool overwrite)
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
  bool Run::hasProperty(const std::string & name) const
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

  void Run::removeProperty(const std::string &name, bool delProperty)
  {
    // Remove any cached entries for this log. Need to make this more general
    for(unsigned int stat = 0; stat < 7; ++stat)
    {
      m_singleValueCache.removeCache(std::make_pair(name,(Math::StatisticType)stat));
    }
    m_manager.removeProperty(name, delProperty);
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Set the good proton charge total for this run
   *  @param charge :: The proton charge in uA.hour
   */
  void Run::setProtonCharge(const double charge)
  {
    if( !hasProperty(PROTON_CHARGE_LOG_NAME) )
    {
      addProperty(PROTON_CHARGE_LOG_NAME, charge, "uA.hour");
    }
    else
    {
      Kernel::Property *charge_prop = getProperty(PROTON_CHARGE_LOG_NAME);
      charge_prop->setValue(boost::lexical_cast<std::string>(charge));
    }
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Retrieve the total good proton charge delivered in this run
   * @return The proton charge in uA.hour
   * @throw Exception::NotFoundError if the proton charge has not been set
   */
  double Run::getProtonCharge() const
  {
    double charge = m_manager.getProperty(PROTON_CHARGE_LOG_NAME);
    return charge;
  }

  //-----------------------------------------------------------------------------------------------
  /**
   * Calculate the total proton charge by integrating up all the entries in the
   * "proton_charge" time series log. This is then saved in the log entry
   * using setProtonCharge().
   * If "proton_charge" is not found, the value is set to 0.0.
   * @return :: the total charge in microAmp*hours.
   */  
  double Run::integrateProtonCharge()
  {
    Kernel::TimeSeriesProperty<double> * log;
    try
    {
      log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( this->getProperty("proton_charge") );
    }
    catch (Exception::NotFoundError &)
    {
      //g_log.information() << "proton_charge log value not found. Total proton charge set to 0.0\n";
      this->setProtonCharge(0);
      return 0;
    }

    if (log)
    {
      double total = log->getTotalValue();
      std::string unit = log->units();
      // Do we need to take account of a unit
      if(unit.find("picoCoulomb") != std::string::npos )
      {
        /// Conversion factor between picoColumbs and microAmp*hours
        const double currentConversion = 1.e-6 / 3600.;
        total *= currentConversion;
      }
      else if(!unit.empty() && unit != "uAh")
      {
        g_log.warning("Proton charge log has units other than uAh or picoCoulombs. The value of the total proton charge has been left at the sum of the log values.");
      }
      this->setProtonCharge(total);
      return total;
    }
    else
    {
      return -1;
    }
  }

  //-----------------------------------------------------------------------------------------------

  /**
   * Store the given values as a set of energy bin boundaries. Throws
   *    - an invalid_argument if fewer than 2 values are given;
   *    - an out_of_range error if first value is greater of equal to the last
   * @param histoBins :: A vector of values that are interpreted as bin boundaries from a histogram
   */
  void Run::storeHistogramBinBoundaries(const std::vector<double> & histoBins)
  {
    if(histoBins.size() < 2)
    {
      std::ostringstream os;
      os << "Run::storeEnergyBinBoundaries - Fewer than 2 values given, size=" << histoBins.size() << ". Cannot interpret values as bin boundaries.";
      throw std::invalid_argument(os.str());
    }
    if(histoBins.front() >= histoBins.back())
    {
      std::ostringstream os;
      os << "Run::storeEnergyBinBoundaries - Inconsistent start & end values given, size=" << histoBins.size() << ". Cannot interpret values as bin boundaries.";
      throw std::out_of_range(os.str());
    }
    m_histoBins = histoBins;
  }

  /**
   * Returns the energy bin boundaries for a given energy value if they have been stored here. Throws a std::runtime_error
   * if the energy bins have not been set and a std::out_of_range error if the input value is out of the stored range
   * @return The bin boundaries for the given energy value
   */
  std::pair<double, double> Run::histogramBinBoundaries(const double value) const
  {
    if(m_histoBins.empty())
    {
      throw std::runtime_error("Run::histogramBoundaries - No energy bins have been stored for this run");
    }

    if(value < m_histoBins.front())
    {
      std::ostringstream os;
      os << "Run::histogramBinBoundaries- Value lower than first bin boundary. Value= " << value << ", first boundary=" << m_histoBins.front();
      throw std::out_of_range(os.str());
    }
    if(value > m_histoBins.back())
    {
      std::ostringstream os;
      os << "Run::histogramBinBoundaries- Value greater than last bin boundary. Value= " << value << ", last boundary=" << m_histoBins.back();
      throw std::out_of_range(os.str());
    }
    const int index = VectorHelper::getBinIndex(m_histoBins, value);
    return std::make_pair(m_histoBins[index], m_histoBins[index+1]);
  }

  //-----------------------------------------------------------------------------------------------
  /** Return the total memory used by the run object, in bytes.
   */
  size_t Run::getMemorySize() const
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
  Kernel::TimeSeriesProperty<T> * Run::getTimeSeriesProperty(const std::string & name) const
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
  HeldType Run::getPropertyValueAsType(const std::string & name) const
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
  double Run::getPropertyAsSingleValue(const std::string & name, Kernel::Math::StatisticType statistic) const
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
      PARALLEL_CRITICAL(Run_getPropertyAsSingleValue)
      {
        // Put it in the cache
        m_singleValueCache.setCache(key, singleValue);
      }
    }
    return singleValue;
  }

  /**
   * Get a pointer to a property by name
   * @param name :: The name of a property, throws an Exception::NotFoundError if it does not exist
   * @return A pointer to the named property
   */
  Kernel::Property * Run::getProperty(const std::string & name) const
  {
    return m_manager.getProperty(name);
  }

  /** Clear out the contents of all logs of type TimeSeriesProperty.
   *  Single-value properties will be left unchanged.
   *
   *  The method has been fully implemented here instead of as a pass-through to
   *  PropertyManager to limit its visibility to Run clients.
   */
  void Run::clearTimeSeriesLogs()
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

  //-----------------------------------------------------------------------------------------------
  /**
   * Set the gonoimeter & optionally read the values from the logs
   * @param goniometer :: A refernce to a goniometer
   * @param useLogValues :: If true, recalculate the goniometer using the log values
   */
  void Run::setGoniometer(const Geometry::Goniometer & goniometer, const bool useLogValues)
  {
    Geometry::Goniometer old = m_goniometer;
    try
    {
      m_goniometer = goniometer; //copy it in
      if(useLogValues) calculateGoniometerMatrix();
    }
    catch(std::runtime_error&)
    {
      m_goniometer = old;
      throw;
    }
  }

  /** Get the gonimeter rotation matrix, calculated using the
   * previously set Goniometer object as well as the angles
   * loaded in the run (if any).
   *
   * As of now, it uses the MEAN angle.
   *
   * @return 3x3 double rotation matrix
   */
  const Mantid::Kernel::DblMatrix & Run::getGoniometerMatrix() const
  {
    return m_goniometer.getR();
  }

  //--------------------------------------------------------------------------------------------
  /** Save the object to an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to create
   */
  void Run::saveNexus(::NeXus::File * file, const std::string & group) const
  {
    file->makeGroup(group, "NXgroup", 1);
    file->putAttr("version", 1);

    // Now the goniometer
    m_goniometer.saveNexus(file, GONIOMETER_LOG_NAME);

    // Now the histogram bins, if there are any
    if(!m_histoBins.empty())
    {
      file->makeGroup(HISTO_BINS_LOG_NAME, "NXdata", 1);
      file->writeData("value", m_histoBins);
      file->closeGroup();
    }

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
    file->closeGroup();
  }

  //--------------------------------------------------------------------------------------------
  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to open. Empty string to NOT open a group, but
   * load any NXlog in the current open group.
   */
  void Run::loadNexus(::NeXus::File * file, const std::string & group)
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
            m_manager.removeProperty(prop->name() );
          m_manager.declareProperty(prop);
        }
      }
      else if (name_class.second == "NXpositioner")
      {
        // Goniometer class
        m_goniometer.loadNexus(file, name_class.first);
      }
      else if(name_class.first == HISTO_BINS_LOG_NAME)
      {
        file->openGroup(name_class.first, "NXdata");
        file->readData("value",m_histoBins);
        file->closeGroup();
      }
      else if (name_class.first == "proton_charge")
      {
        // Old files may have a proton_charge field, single value (not even NXlog)
        double charge;
        file->readData("proton_charge", charge);
        this->setProtonCharge(charge);
      }
    }
    if (!group.empty()) file->closeGroup();


    if( this->hasProperty("proton_charge") )
    {
      // Old files may have a proton_charge field, single value.
      // Modern files (e.g. SNS) have a proton_charge TimeSeriesProperty.
      PropertyWithValue<double> *charge_log = dynamic_cast<PropertyWithValue<double>*>(this->getProperty("proton_charge"));
      if (charge_log)
      {  this->setProtonCharge(boost::lexical_cast<double>(charge_log->value()));
      }
    }
  }

  //-----------------------------------------------------------------------------------------------------------------------
  // Private methods
  //-----------------------------------------------------------------------------------------------------------------------

  /** Adds all the time series in the second property manager to those in the first
  * @param sum the properties to add to
  * @param toAdd the properties to add
  */
  void Run::mergeMergables(Mantid::Kernel::PropertyManager & sum, const Mantid::Kernel::PropertyManager & toAdd)
  {
    // get pointers to all the properties on the right-handside and prepare to loop through them
    const std::vector<Property*> inc = toAdd.getProperties();
    std::vector<Property*>::const_iterator end = inc.end();
    for (std::vector<Property*>::const_iterator it=inc.begin(); it != end;++it)
    {
      const std::string rhs_name = (*it)->name();
      try
      {
        //now get pointers to the same properties on the left-handside
        Property * lhs_prop(sum.getProperty(rhs_name));
        lhs_prop->merge(*it);
      }
      catch (Exception::NotFoundError &)
      {
        //copy any properties that aren't already on the left hand side
        Property * copy = (*it)->clone();
        //And we add a copy of that property to *this
        sum.declareProperty(copy, "");
      }
    }
  }


  /**
   * Calculate the goniometer matrix
   */
  void Run::calculateGoniometerMatrix()
  {
    for (size_t i=0; i < m_goniometer.getNumberAxes(); ++i)
    {
      const double angle = getLogAsSingleValue(m_goniometer.getAxis(i).name, Kernel::Math::Mean);
      m_goniometer.setRotationAngle(i, angle);
    }
  }


  /// @cond
  /// Macro to instantiate concrete template members
#define INSTANTIATE(TYPE) \
  template MANTID_API_DLL Kernel::TimeSeriesProperty<TYPE> * Run::getTimeSeriesProperty(const std::string &) const;\
  template MANTID_API_DLL TYPE Run::getPropertyValueAsType(const std::string &) const;

  INSTANTIATE(double);
  INSTANTIATE(int);
  INSTANTIATE(std::string);
  INSTANTIATE(bool);
  /// @endcond

} //API namespace

}

