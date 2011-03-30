//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid
{
  namespace API
  {
    /** 
     * Constructor
     */
    AlgorithmHasProperty::AlgorithmHasProperty(const std::string & propName) 
      : m_propName(propName)
    {
    }
    
    /** 
     * Destructor
     */
    AlgorithmHasProperty::~AlgorithmHasProperty()
    {    
    }
  
    /** 
     * Checks the value based on the validator's rules
     * @param value :: The input algorithm to check
     * @returns An error message to display to users or an empty string on no error
     */
    std::string AlgorithmHasProperty::checkValidity(const IAlgorithm_sptr & value) const
    {
      std::string message("");
      if( value->existsProperty(m_propName) )
      {
	Kernel::Property *p = value->getProperty(m_propName);
	if( !p->isValid().empty() )
	{
	  message = "Algorithm object contains the required property \""
	    +  m_propName + "\" but it has an invalid value: " + p->value();
	}
      }
      else
      {
	message = "Algorithm object does not have the required property \"" +  m_propName + "\"";
      }

      return message;
    }

  } // namespace Mantid
} // namespace API

