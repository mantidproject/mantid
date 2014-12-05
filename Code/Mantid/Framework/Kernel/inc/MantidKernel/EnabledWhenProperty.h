#ifndef MANTID_KERNEL_ENABLEDWHENPROPERTY_H_
#define MANTID_KERNEL_ENABLEDWHENPROPERTY_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"


namespace Mantid
{
namespace Kernel
{

  /** Enum for use in EnabledWhenProperty */
  enum ePropertyCriterion
  {
    IS_DEFAULT,
    IS_NOT_DEFAULT,
    IS_EQUAL_TO,
    IS_NOT_EQUAL_TO,
    IS_MORE_OR_EQ
  };

  /** IPropertySettings for a property that sets it to enabled (in the GUI)
     when the value of another property is:
      - its default (or not)
      - equal to a string (or not)

      Usage:

        - In an algorithm's init() method, after a call to create a property:

        declareProperty("PropA", 123);

        - Add a call like this:

        setPropertySettings("PropA", new EnabledWhenProperty("OtherProperty", IS_EQUAL_TO, "2000");

        - This will make the property "PropA" show as enabled when "OtherProperty"'s value is equal to "2000". Similarly, you can use:

        setPropertySettings("PropA", new VisibleWhenProperty("OtherProperty", IS_NOT_DEFAULT);

        - This will make the property "PropA" show as visible when "OtherProperty" is NOT the default value for it.


    @author Janik Zikovsky
    @date 2011-08-25

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport EnabledWhenProperty : public IPropertySettings
  {
  public:
    //--------------------------------------------------------------------------------------------
    /** Constructor
     * @param otherPropName :: Name of the OTHER property that we will check.
     * @param when :: Criterion to evaluate
     * @param value :: For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value (as string) to check for
     */
    EnabledWhenProperty(std::string otherPropName,
                        ePropertyCriterion when, std::string value = "")
    : IPropertySettings(),
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
    virtual bool fulfillsCriterion(const IPropertyManager * algo) const
    {
      // Find the property
      if (algo == NULL) return true;
      Property * prop = NULL;
      try
      {
        prop = algo->getPointerToProperty(m_otherPropName);
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
      case IS_MORE_OR_EQ:{
          int check = boost::lexical_cast<int>(m_value);
          int iPropV= boost::lexical_cast<int>(propValue);
          return (iPropV>=check);}
      default:
        // Unknown criterion
        return true;
      }
    }

    //--------------------------------------------------------------------------------------------
    /// Return true/false based on whether the other property satisfies the criterion
    virtual bool isEnabled(const IPropertyManager * algo) const
    {
      return fulfillsCriterion(algo);
    }

    //--------------------------------------------------------------------------------------------
    /// Return true always
    virtual bool isVisible(const IPropertyManager * ) const
    {
      return true;
    }
    /// does nothing in this case and put here to satisfy the interface.
    void modify_allowed_values(Property * const){}
    //--------------------------------------------------------------------------------------------
    /// Make a copy of the present type of validator
    virtual IPropertySettings * clone()
    {
      EnabledWhenProperty * out = new EnabledWhenProperty(m_otherPropName, m_when, m_value);
      return out;
    }

  protected:
    /// Name of the OTHER property that we will check.
    std::string m_otherPropName;
    /// Criterion to evaluate
    ePropertyCriterion m_when;
    /// For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value (as string) to check for
    std::string m_value;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_ENABLEDWHENPROPERTY_H_ */
