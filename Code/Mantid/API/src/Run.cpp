//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Run.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "boost/lexical_cast.hpp"

namespace Mantid
{
namespace API
{

using namespace Kernel;

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
   * @param copy The object to initialize the copy from
   */
  Run::Run(const Run& copy) : m_manager(copy.m_manager), m_protonChargeName(copy.m_protonChargeName)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Assignment operator
   * @param rhs The object whose properties should be copied into this
   * @returns A cont reference to the copied object
   */
  const Run& Run::operator=(const Run& rhs)
  {
    if( this == &rhs ) return *this;
    m_manager = rhs.m_manager;
    return *this;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Addition operator
   * @param rhs The object that is being added to this.
   * @returns A reference to the summed object
   */
  Run& Run::operator+=(const Run& rhs)
  {
    //The propery manager operator will have to handle it
    m_manager += rhs.m_manager;
    return *this;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
   *  absolute time range.
   *
   * Total proton charge will get re-integrated after filtering.
   *
   * @param start Absolute start time. Any log entries at times >= to this time are kept.
   * @param stop Absolute stop time. Any log entries at times < than this time are kept.
   */
  void Run::filterByTime(const Kernel::dateAndTime start, const Kernel::dateAndTime stop)
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
   * @param splitter TimeSplitterType with the intervals and destinations.
   * @param outputs Vector of output runs.
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
   * @param prop A pointer to a property whose ownership is transferred to this object
   * @param overwrite If true, a current value is overwritten. (Default: False)
   */
  void Run::addProperty(Kernel::Property *prop, bool overwrite)
  {
    // Mmake an exception for the proton charge
    // and overwrite it's value as we don't want to store the proton charge in two separate locations
    std::string name = prop->name();
    if( hasProperty(name) && (overwrite || prop->name() == m_protonChargeName) )
    {
      removeProperty(name);
    }
    m_manager.declareProperty(prop, "");
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Set the good proton charge total for this run
   *  @param charge The proton charge in uA.hour
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
   * @throws Exception::NotFoundError if the proton charge has not been set
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
   *
   * @return the total charge in microAmp*hours
   */
  double Run::integrateProtonCharge()
  {
    /// Conversion factor between picoColumbs and microAmp*hours
    const double CURRENT_CONVERSION = 1.e-6 / 3600.;

    Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( this->getProperty("proton_charge") );
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




} //API namespace

}

