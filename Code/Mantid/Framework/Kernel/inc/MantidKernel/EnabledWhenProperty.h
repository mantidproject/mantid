#ifndef MANTID_KERNEL_ENABLEDWHENPROPERTY_H_
#define MANTID_KERNEL_ENABLEDWHENPROPERTY_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/IPropertyManager.h"


namespace Mantid
{
namespace Kernel
{

  /** Enum for use in EnabledWhenProperty */
  enum eValidatorCriterion
  {
    IS_DEFAULT,
    IS_NOT_DEFAULT,
    IS_EQUAL_TO,
    IS_NOT_EQUAL_TO
  };

  /** Validator for a property that sets it to enabled (in the GUI)
   * when the value of another property is:
   *  - its default (or not)
   *  - equal to a string (or not)
    
    @author Janik Zikovsky
    @date 2011-08-25

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  template <typename TYPE>
  class DLLExport EnabledWhenProperty : public IValidator<TYPE>
  {
  public:
    //--------------------------------------------------------------------------------------------
    /** Constructor
     * @param algo :: ptr to the calling Algorithm (type IPropertyManager)
     * @param otherPropName :: Name of the OTHER property that we will check.
     * @param when :: Criterion to evaluate
     * @param value :: For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value (as string) to check for
     */
    EnabledWhenProperty(const IPropertyManager * algo, std::string otherPropName,
                        eValidatorCriterion when, std::string value = "")
    : IValidator<TYPE>(algo),
      m_otherPropName(otherPropName), m_when(when), m_value(value)
    {
    }

    /// Destructor
    virtual ~EnabledWhenProperty()
    {}
    

    //--------------------------------------------------------------------------------------------
    /** Does the validator fulfill the criterion based on the
     * other property values?
     * @return true if fulfilled or if any problem was found (missing property, e.g.).
     */
    virtual bool fulfillsCriterion() const
    {
      // Find the property
      if (this->m_propertyManager == NULL) return true;
      Property * prop = NULL;
      try
      {
        prop = this->m_propertyManager->getPointerToProperty(m_otherPropName);
      }
      catch (Exception::NotFoundError&)
      {
        return true; //Property not found. Ignore
      }
      if (!prop) return true;

      // Value of the other property
      std::string propValue = prop->value();

      // OK, we have the property. Check the condition
      switch(m_when)
      {
      case IS_DEFAULT:
        return prop->isDefault();
      case IS_NOT_DEFAULT:
        return !prop->isDefault();
      case IS_EQUAL_TO:
        return (propValue == m_value);
      case IS_NOT_EQUAL_TO:
        return (propValue != m_value);
      default:
        // Unknown criterion
        return true;
      }
    }

    //--------------------------------------------------------------------------------------------
    /// Return the chosen value if the other property is/isn't default
    virtual bool isEnabled() const
    {
      return fulfillsCriterion();
    }

    //--------------------------------------------------------------------------------------------
    /// Return true always
    virtual bool isVisible() const
    {
      return true;
    }

    //--------------------------------------------------------------------------------------------
    /// Make a copy of the present type of validator
    virtual IValidator<TYPE>* clone()
    {
      EnabledWhenProperty<TYPE> * out = new EnabledWhenProperty<TYPE>(this->m_propertyManager, m_otherPropName, m_when, m_value);
      return out;
    }

  protected:

    /** Checks the value based on the validator's rules */
    virtual std::string checkValidity(const TYPE &) const
    { return ""; }

  protected:
    /// Name of the OTHER property that we will check.
    std::string m_otherPropName;
    /// Criterion to evaluate
    eValidatorCriterion m_when;
    /// For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value (as string) to check for
    std::string m_value;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_ENABLEDWHENPROPERTY_H_ */
