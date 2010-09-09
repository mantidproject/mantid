//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Run.h"
#include "boost/lexical_cast.hpp"

namespace Mantid
{
namespace API
{
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
   * Add data to the object in the form of a property
   * @param prop A pointer to a property whose ownership is transferred to this object
   */
  void Run::addProperty(Kernel::Property *prop)
  {
    // Throws if the property exists already but make an exception for the proton charge
    // and overwrite it's value as we don't want to store the proton charge in two separate locations
    if( prop->name() == m_protonChargeName && hasProperty(m_protonChargeName) )
    {
      removeProperty(m_protonChargeName);
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
      addProperty(m_protonChargeName, charge);
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



} //API namespace

}

