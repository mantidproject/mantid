//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Run.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

using namespace Kernel;

const int Run::ADDABLES = 6;
const std::string Run::ADDABLE[ADDABLES] = {"tot_prtn_chrg", "rawfrm", "goodfrm", "dur", "gd_prtn_chrg", "uA.hour"};

  //----------------------------------------------------------------------
  // Public member functions
  //----------------------------------------------------------------------
  /**
   * Default constructor
   */
  Run::Run() : m_manager(), m_protonChargeName("gd_prtn_chrg")
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
  Run::Run(const Run& copy) : m_manager(copy.m_manager), m_protonChargeName(copy.m_protonChargeName), m_goniometer(copy.m_goniometer)
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
    m_protonChargeName = rhs.m_protonChargeName;
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
      // get a pointer to the property on the right-handside worksapce
      Property * right;
      try
      {
        right = rhs.m_manager.getProperty(ADDABLE[i]);
      }
      catch (Exception::NotFoundError err)
      {
        //if it's not there then ignore it and move on
        continue;
      }
      // now deal with the left-handside
      Property * left;
      try
      {
        left = m_manager.getProperty(ADDABLE[i]);
      }
      catch (Exception::NotFoundError err)
      {
        //no property on the left-handside, create one and copy the right-handside across verbatum
        m_manager.declareProperty(right->clone(), "");
        continue;
      }
      
      left->operator+=(right);

    }
    return *this;
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
        
/*        TimeSeriesProperty * timeS = dynamic_cast< TimeSeriesProperty * >(lhs_prop);
        if (timeS)
        {
          (*lhs_prop) += (*it);
        }*/
      }
      catch (Exception::NotFoundError err)
      {
        //copy any properties that aren't already on the left hand side
        Property * copy = (*it)->clone();
        //And we add a copy of that property to *this
        sum.declareProperty(copy, "");
      }
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
   * Add data to the object in the form of a property
   * @param prop :: A pointer to a property whose ownership is transferred to this object
   * @param overwrite :: If true, a current value is overwritten. (Default: False)
   */
  void Run::addProperty(Kernel::Property *prop, bool overwrite)
  {
    // Mmake an exception for the proton charge
    // and overwrite it's value as we don't want to store the proton charge in two separate locations
    // Similar we don't want more than one run_title
    std::string name = prop->name();
    if( hasProperty(name) && (overwrite || prop->name() == m_protonChargeName || prop->name()=="run_title") )
    {
      removeProperty(name);
    }
    m_manager.declareProperty(prop, "");
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Set the good proton charge total for this run
   *  @param charge :: The proton charge in uA.hour
   */
  void Run::setProtonCharge(const double charge)
  {
    if( !hasProperty(m_protonChargeName) )
    {
      addProperty(m_protonChargeName, charge, "uA.hour");
    }
    else
    {
      Kernel::Property *charge_prop = getProperty(m_protonChargeName);
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
    double charge = m_manager.getProperty(m_protonChargeName);
    return charge;
  }

  //-----------------------------------------------------------------------------------------------
    /**
   * Calculate the total proton charge by summing up all the entries in the
   * "proton_charge" time series log. This is then saved in the log entry
   * using setProtonCharge().
   * If "proton_charge" is not found, the value is set to 0.0.
   *
   * @return :: the total charge in microAmp*hours.
   */
  double Run::integrateProtonCharge()
  {
    /// Conversion factor between picoColumbs and microAmp*hours
    const double CURRENT_CONVERSION = 1.e-6 / 3600.;
    Kernel::TimeSeriesProperty<double> * log;

    try
    {
      log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( this->getProperty("proton_charge") );
    }
    catch (Exception::NotFoundError e)
    {
      //g_log.information() << "proton_charge log value not found. Total proton charge set to 0.0\n";
      this->setProtonCharge(0);
      return 0;
    }

    if (log)
    {
      double total = log->getTotalValue() * CURRENT_CONVERSION;
      this->setProtonCharge(total);
      return total;
    }
    else
    {
      return -1;
    }
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




} //API namespace

}

