#ifndef MANTID_KERNEL_PROPERTYMANAGER_H_
#define MANTID_KERNEL_PROPERTYMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <map>

#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
namespace Kernel
{
class Logger;
/** @class PropertyManager PropertyManager.h Kernel/PropertyManager.h

 Property manager helper class.
 This class is used by algorithms and services for helping to manage their own set of properties.
 
 N.B. ONCE YOU HAVE DECLARED A PROPERTY TO THE MANAGER IT IS OWNED BY THIS CLASS (I.E. DON'T DELETE IT!)

 Property values of any type except std::string can be extracted using getProperty().
 For assignment of string properties it is necessary to use getPropertyValue().

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class PropertyMgr (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 20/11/2007
 
 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
 
 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport PropertyManager
{
public:
  PropertyManager();
  virtual ~PropertyManager();

  // Function to declare properties (i.e. store them)
  virtual void declareProperty(Property *p);

  // Sets all the declared properties from 
  virtual void setProperties(const std::string &values);
  virtual void setPropertyValue(const std::string &name, const std::string &value);

  virtual bool existsProperty(const std::string &name) const;
  virtual bool validateProperties() const;
  
  virtual std::string getPropertyValue(const std::string &name) const;
  virtual const std::vector< Property*>& getProperties() const;

  /** Templated method to set the value of a PropertyWithValue
   *  @param name The name of the property (case insensitive)
   *  @param value The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
   */
  template <typename T>
  void setProperty(const std::string &name, const T value)
  {
    PropertyWithValue<T> *prop = dynamic_cast<PropertyWithValue<T>*>(getPointerToProperty(name));
    if (prop)
    {
      *prop = value;
    }
    else
    {
      throw std::invalid_argument("Attempt to assign to property of incorrect type");
    }
  }
  
  /// Specialised version of setProperty template method
  void setProperty(const std::string &name, const char* value)
  {
    this->setPropertyValue(name, std::string(value));
  }

protected:
  /** Add a property of the template type to the list of managed properties
   *  @param name The name to assign to the property
   *  @param value The initial value to assign to the property
   *  @param validator Pointer to the (optional) validator. Ownership will be taken over.
   *  @param doc The (optional) documentation string
   *  @throw Exception::ExistsError if a property with the given name already exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
  template <typename T>
  void declareProperty(const std::string &name, T value,
                       IValidator<T> *validator = new NullValidator<T>, const std::string &doc="")
  {
    Property *p = new PropertyWithValue<T>(name, value, validator);
    p->setDocumentation(doc);
    declareProperty(p);
  }

  // Specialised version of above function
  virtual void declareProperty(const std::string &name, const char* value,
      IValidator<std::string> *validator = new NullValidator<std::string>, const std::string &doc="");

private:
  /// Private copy constructor.
  PropertyManager(const PropertyManager&);
  /// Private copy assignment operator.
  PropertyManager& operator=(const PropertyManager&);

  /// typedef for the map holding the properties
  typedef std::map<std::string, Property*> PropertyMap;
  /// The properties under management
  PropertyMap m_properties;
  /// Stores the order that the properties were declared in
  std::vector<Property*> m_orderedProperties;

  /// Static reference to the logger class
  static Logger& g_log;

  Property* getPointerToProperty(const std::string &name) const;

  /** Templated method to get the value of a property
   *  @param name The name of the property (case insensitive)
   *  @return The value of the property
   *  @throw std::runtime_error If an attempt is made to assign a property to a different type
   *  @throw Exception::NotFoundError If the property requested does not exist
   */
  template<typename T>
  T getValue(const std::string &name) const;
  /// Utility class that enables the getProperty() method to effectively be templated on the return type
  struct TypedValue
  {
    /// Reference to the containing PropertyManager
    const PropertyManager& pm;
    /// The name of the property desired
    const std::string prop;

    /// Constructor
    TypedValue(const PropertyManager& p, const std::string &name) : pm(p), prop(name) {}

    /// Templated cast operator so that a TypedValue can be cast to what is actually wanted
    template<typename T> 
    operator T() { return pm.getValue<T>(prop); }
  };

public:
  /// Get the value of a property
  virtual TypedValue getProperty(const std::string &name) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYMANAGER_H_*/
