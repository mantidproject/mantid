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
    Run::Run() : PropertyManager(), m_protonChargeName("proton_charge_tot")
    {
    }

    /**
     * Destructor
     */
    Run::~Run()
    {
    }

    /**
     * Add data to the object in the form of a property
     * @param prop A pointer to a property whose ownership is transferred to this object
     */
    void Run::addProperty(Kernel::Property *prop)
    {
      this->declareProperty(prop, "");
    }
    
    /** 
     * Set the good proton charge total for this run
     *  @param charge The proton charge in uA.hour
     */
    void Run::setProtonCharge(const double charge)
    {
      if( !hasProperty(m_protonChargeName) )
      {
	addProperty(new Kernel::PropertyWithValue<double>(m_protonChargeName, charge));
      }
      else
      {
	Kernel::Property *charge_prop = getProperty(m_protonChargeName);
	charge_prop->setValue(boost::lexical_cast<std::string>(charge));
      }
    }

    /** 
     * Retrieve the total good proton charge delivered in this run
     * @return The proton charge in uA.hour
     * @throws Exception::NotFoundError if the proton charge has not been set
     */
    double Run::getProtonCharge() const
    {
      double charge = PropertyManager::getProperty(m_protonChargeName);
      return charge;
    }

  }
  
}

