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
#include <numeric>

namespace Mantid
{
namespace API
{

using namespace Kernel;

namespace
{
  /// The number of log entries summed when adding a run
  const int ADDABLES = 12;
  /// The names of the log entries summed when adding two runs together
  const std::string ADDABLE[ADDABLES] = {"tot_prtn_chrg", "rawfrm", "goodfrm", "dur", "gd_prtn_chrg", "uA.hour",
                                         "monitor0_counts", "monitor1_counts", "monitor2_counts", "monitor3_counts",
                                         "monitor4_counts", "monitor5_counts"};
  /// Name of the goniometer log when saved to a NeXus file
  const char * GONIOMETER_LOG_NAME = "goniometer";
  /// Name of the stored histogram bins log when saved to NeXus
  const char * HISTO_BINS_LOG_NAME = "processed_histogram_bins";
  const char * PEAK_RADIUS_GROUP = "peak_radius";
  const char * INNER_BKG_RADIUS_GROUP = "inner_bkg_radius";
  const char * OUTER_BKG_RADIUS_GROUP = "outer_bkg_radius";

  /// static logger object
  Kernel::Logger g_log("Run");

}

  //----------------------------------------------------------------------
  // Public member functions
  //----------------------------------------------------------------------
  /**
   * Default constructor
   */
  Run::Run() : m_goniometer(), m_histoBins()
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
  Run::Run(const Run& copy) : LogManager(copy),
    m_goniometer(copy.m_goniometer)
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
    LogManager::filterByTime(start,stop);
    //Re-integrate proton charge
    this->integrateProtonCharge();
  }

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

    // Other properties are added together if they are on the approved list
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


  //-----------------------------------------------------------------------------------------------
  /**
   * Split a run by time (splits the TimeSeriesProperties contained).
   *
   * Total proton charge will get re-integrated after filtering.
   *
   * @param splitter :: TimeSplitterType with the intervals and destinations.
   * @param outputs :: Vector of output runs.
   */
  void Run::splitByTime(TimeSplitterType& splitter, std::vector< LogManager * > outputs) const
  {
 
    //std::vector<LogManager *> outputsBase(outputs.begin(),outputs.end());
    LogManager::splitByTime(splitter,outputs);

    size_t n = outputs.size();
    //Re-integrate proton charge of all outputs
    for (size_t i=0; i<n; i++)
    {
      if (outputs[i])
      {
        auto run = dynamic_cast<Run *>(outputs[i]);
        if(run)run->integrateProtonCharge();
      }
    }

  }

 


  //-----------------------------------------------------------------------------------------------
  /**
   * Set the good proton charge total for this run
   *  @param charge :: The proton charge in uA.hour
   */
  void Run::setProtonCharge(const double charge)
  {
    const std::string PROTON_CHARGE_UNITS("uA.hour");
    if( !hasProperty(PROTON_CHARGE_LOG_NAME) )
    {
      addProperty(PROTON_CHARGE_LOG_NAME, charge, PROTON_CHARGE_UNITS);
    }
    else
    {
      Kernel::Property *charge_prop = getProperty(PROTON_CHARGE_LOG_NAME);
      charge_prop->setValue(boost::lexical_cast<std::string>(charge));
      charge_prop->setUnits(PROTON_CHARGE_UNITS);
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
      this->setProtonCharge(0);
      return 0;
    }

    if (log)
    {
      const std::vector<double> logValues = log->valuesAsVector();
      double total = std::accumulate(logValues.begin(), logValues.end(), 0.0);
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

  /**
   * Returns the energy bin boundaries. Throws a std::runtime_error
   * if the energy bins have not been set.
   * @return The bin boundaries vector
   */
  std::vector<double> Run::getBinBoundaries() const
  {
    if(m_histoBins.empty())
    {
      throw std::runtime_error("Run::histogramBoundaries - No energy bins have been stored for this run");
    }

    return m_histoBins;
  }

  //-----------------------------------------------------------------------------------------------
  /** Return the total memory used by the run object, in bytes.
   */
  size_t Run::getMemorySize() const
  {
    size_t total = LogManager::getMemorySize();
    total += sizeof(m_goniometer);
    total += m_histoBins.size()*sizeof(double);
    return total;
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
   * @param keepOpen :: If true, leave the file open after saving
   */
  void Run::saveNexus(::NeXus::File * file, const std::string & group,bool keepOpen) const
  {
    LogManager::saveNexus(file,group,true);

    // write the goniometer
    m_goniometer.saveNexus(file, GONIOMETER_LOG_NAME);

    // write the histogram bins, if there are any
    if(!m_histoBins.empty())
    {
      file->makeGroup(HISTO_BINS_LOG_NAME, "NXdata", 1);
      file->writeData("value", m_histoBins);
      file->closeGroup();
    }
    if( this->hasProperty("PeakRadius") )
    {
    	const std::vector<double> & values =
    			this->getPropertyValueAsType<std::vector<double> >("PeakRadius");

        file->makeGroup(PEAK_RADIUS_GROUP, "NXdata", 1);
        file->writeData("value", values);
        file->closeGroup();
    }
    if( this->hasProperty("BackgroundInnerRadius") )
    {
        file->makeGroup(INNER_BKG_RADIUS_GROUP, "NXdata", 1);
       	const std::vector<double> & values =
        		this->getPropertyValueAsType<std::vector<double> >("BackgroundInnerRadius");
        file->writeData("value", values);
        file->closeGroup();
    }
    if( this->hasProperty("BackgroundOuterRadius") )
    {
        file->makeGroup(OUTER_BKG_RADIUS_GROUP, "NXdata", 1);
       	const std::vector<double> & values =
        		this->getPropertyValueAsType<std::vector<double> >("BackgroundOuterRadius");
        file->writeData("value", values);
        file->closeGroup();
    }
    if(!keepOpen)file->closeGroup();
  }

  //--------------------------------------------------------------------------------------------
  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to open. Empty string to NOT open a group, but
   * load any NXlog in the current open group.
   * @param keepOpen :: If true, then the file is left open after doing to load
   */
  void Run::loadNexus(::NeXus::File * file, const std::string & group, bool keepOpen)
  {
    LogManager::loadNexus(file,group,true);

    std::map<std::string, std::string> entries;
    file->getEntries(entries);
    std::map<std::string, std::string>::iterator it = entries.begin();
    std::map<std::string, std::string>::iterator it_end = entries.end();
    for (; it != it_end; ++it)
    {
      // Get the name/class pair
      const std::pair<std::string, std::string> & name_class = *it;
      if (name_class.second == "NXpositioner")
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
      else if(name_class.first == PEAK_RADIUS_GROUP)
      {
        file->openGroup(name_class.first, "NXdata");
        std::vector<double> values;
        file->readData("value",values);
        file->closeGroup();
        this->addProperty("PeakRadius",values, true);
      }
      else if(name_class.first == INNER_BKG_RADIUS_GROUP)
      {
        file->openGroup(name_class.first, "NXdata");
        std::vector<double> values;
        file->readData("value",values);
        file->closeGroup();
        this->addProperty("BackgroundInnerRadius",values, true);
      }
      else if(name_class.first == OUTER_BKG_RADIUS_GROUP)
      {
        file->openGroup(name_class.first, "NXdata");
        std::vector<double> values;
        file->readData("value",values);
        file->closeGroup();
        this->addProperty("BackgroundOuterRadius",values, true);
      }
      else if (name_class.first == "proton_charge" && !this->hasProperty("proton_charge"))
      {
        // Old files may have a proton_charge field, single value (not even NXlog)
        double charge;
        file->readData("proton_charge", charge);
        this->setProtonCharge(charge);
      }
    }
    if (!(group.empty() || keepOpen)) file->closeGroup();


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

  /**
   * Calculate the goniometer matrix
   */
  void Run::calculateGoniometerMatrix()
  {
    for (size_t i=0; i < m_goniometer.getNumberAxes(); ++i)
    {
  	  const std::string axisName = m_goniometer.getAxis(i).name;
	  const double minAngle = getLogAsSingleValue(axisName, Kernel::Math::Minimum);
	  const double maxAngle = getLogAsSingleValue(axisName, Kernel::Math::Maximum);
	  const double angle = getLogAsSingleValue(axisName, Kernel::Math::Mean);
	  if(minAngle != maxAngle)
      {
		  const double lastAngle = getLogAsSingleValue(axisName, Kernel::Math::LastValue);
		  g_log.warning("Goniometer angle changed in " + axisName + " log from " + boost::lexical_cast<std::string>(minAngle)
				  + " to " + boost::lexical_cast<std::string>(maxAngle) + ".  Used mean = " + boost::lexical_cast<std::string>(angle) +".");
		  if (axisName.compare("omega") == 0)
		  {
			  g_log.warning("To set to last angle, replace omega with "
					  + boost::lexical_cast<std::string>(lastAngle) + ": SetGoniometer(Workspace=\'workspace\',Axis0=omega,0,1,0,1\',Axis1='chi,0,0,1,1',Axis2='phi,0,1,0,1')");
		  }
		  else if (axisName.compare("chi") == 0)
		  {
			  g_log.warning("To set to last angle, replace chi with "
					  + boost::lexical_cast<std::string>(lastAngle) + ": SetGoniometer(Workspace=\'workspace\',Axis0=omega,0,1,0,1\',Axis1='chi,0,0,1,1',Axis2='phi,0,1,0,1')");
		  }
		  else if (axisName.compare("phi") == 0)
		  {
			  g_log.warning("To set to last angle, replace phi with "
					  + boost::lexical_cast<std::string>(lastAngle) + ": SetGoniometer(Workspace=\'workspace\',Axis0=omega,0,1,0,1\',Axis1='chi,0,0,1,1',Axis2='phi,0,1,0,1')");
		  }
	  }
	  m_goniometer.setRotationAngle(i, angle);
    }
  }

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

} //API namespace

}

