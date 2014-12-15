#ifndef MANTID_KERNEL_IPROPERTYMANAGER_H_
#define MANTID_KERNEL_IPROPERTYMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"
#include <vector>
#include <map>

namespace Mantid
{

namespace Kernel
{

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class Logger;
class DataItem;
class DateAndTime;
class PropertyManager;
template<typename T> class TimeSeriesProperty;
template<typename T> class Matrix;

/** @class IPropertyManager IPropertyManager.h Kernel/IPropertyManager.h

 Interface to PropertyManager

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class PropertyMgr (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 20/11/2007
 @author Roman Tolchenov, Tessella plc
 @date 02/03/2009

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_KERNEL_DLL IPropertyManager
{
public:
    //IPropertyManager(){}
    virtual ~IPropertyManager(){}

    /// Function to declare properties (i.e. store them)
    virtual void declareProperty(Property *p, const std::string &doc="" ) = 0;

    /// Removes the property from management
    virtual void removeProperty(const std::string &name, const bool delproperty=true) = 0;

    /** Sets all the declared properties from a string.
        @param propertiesArray :: A list of name = value pairs separated by a semicolon
     */
    virtual void setProperties(const std::string &propertiesArray) = 0;

    /** Sets property value from a string
        @param name :: Property name
        @param value :: New property value
     */
    virtual void setPropertyValue(const std::string &name, const std::string &value) = 0;

    /// Set the value of a property by an index
    virtual void setPropertyOrdinal(const int &index, const std::string &value) = 0;

    /// Checks whether the named property is already in the list of managed property.
    virtual bool existsProperty(const std::string &name) const = 0;

    /// Validates all the properties in the collection
    virtual bool validateProperties() const = 0;
    /// Returns the number of properties under management
    virtual size_t propertyCount() const = 0;
    /// Get the value of a property as a string
    virtual std::string getPropertyValue(const std::string &name) const = 0;

    /// Get the list of managed properties.
    virtual const std::vector< Property*>& getProperties() const = 0;

    /** Templated method to set the value of a PropertyWithValue
     *  @param name :: The name of the property (case insensitive)
     *  @param value :: The value to assign to the property
     *  @throw Exception::NotFoundError If the named property is unknown
     *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
     */
    template <typename T>
    IPropertyManager* setProperty(const std::string &name, const T & value)
    {
      setTypedProperty(name, value, boost::is_convertible<T, boost::shared_ptr<DataItem> >());
      this->afterPropertySet(name);
      return this;
    }
    
    /** Specialised version of setProperty template method to handle const char *
    *  @param name :: The name of the property (case insensitive)
    *  @param value :: The value to assign to the property
    *  @throw Exception::NotFoundError If the named property is unknown
    *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
    */
    IPropertyManager* setProperty(const std::string &name, const char* value)
    {
      this->setPropertyValue(name, std::string(value));
      return this;
    }

    /** Specialised version of setProperty template method to handle std::string
    *  @param name :: The name of the property (case insensitive)
    *  @param value :: The value to assign to the property
    *  @throw Exception::NotFoundError If the named property is unknown
    *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
    */
    IPropertyManager* setProperty(const std::string &name, const std::string & value)
    {
      this->setPropertyValue(name, value);
      return this;
    }

    /// Update values of the existing properties.
    void updatePropertyValues( const IPropertyManager &other );

    /// Return the property manager serialized as a string.
    virtual std::string asString(bool withDefaultValues = false, char separator=',') const = 0;

    /** Give settings to a property to determine when it gets enabled/hidden.
     * Passes ownership of the given IPropertySettings object to the named property
     * @param name :: property name
     * @param settings :: IPropertySettings     */
    void setPropertySettings(const std::string &name, IPropertySettings * settings)
    {
      Property * prop = getPointerToProperty(name);
      if (prop) prop->setSettings(settings);
    }
    /** Set the group for a given property
     * @param name :: property name
     * @param group :: Name of the group it belongs to     */
    void setPropertyGroup(const std::string &name, const std::string & group)
    {
      Property * prop = getPointerToProperty(name);
      if (prop) prop->setGroup(group);
    }

    /// Get the list of managed properties in a given group.
    std::vector< Property*> getPropertiesInGroup(const std::string& group) const;

    virtual void filterByTime(const DateAndTime &/*start*/, const DateAndTime &/*stop*/) = 0;
    virtual void splitByTime(std::vector<SplittingInterval>& /*splitter*/, std::vector< PropertyManager * >/* outputs*/) const = 0;
    virtual void filterByProperty(const TimeSeriesProperty<bool> & /*filte*/) =0;

protected:

    /** Add a property of the template type to the list of managed properties
    *  @param name :: The name to assign to the property
    *  @param value :: The initial value to assign to the property
    *  @param validator :: Pointer to the (optional) validator.
    *  @param doc :: The (optional) documentation string
    *  @param direction :: The (optional) direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    template <typename T>
    void declareProperty(const std::string &name, T value,
                         IValidator_sptr validator = IValidator_sptr(new NullValidator),
                         const std::string &doc="", const unsigned int direction = Direction::Input)
    {
        Property *p = new PropertyWithValue<T>(name, value, validator, direction);
        declareProperty(p, doc);
    }

    /** Add a property to the list of managed properties with no validator
    *  @param name :: The name to assign to the property
    *  @param value :: The initial value to assign to the property
    *  @param doc :: The documentation string
    *  @param direction :: The (optional) direction of the property, in (default), out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    template <typename T>
    void declareProperty(const std::string &name, T value, const std::string &doc,
      const unsigned int direction = Direction::Input)
    {
      Property *p = new PropertyWithValue<T>(name, value, boost::make_shared<NullValidator>(), direction);
      declareProperty(p, doc);
    }

    /** Add a property of the template type to the list of managed properties
    *  @param name :: The name to assign to the property
    *  @param value :: The initial value to assign to the property
    *  @param direction :: The direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    template <typename T>
    void declareProperty(const std::string &name, T value, const unsigned int direction)
    {
      Property *p = new PropertyWithValue<T>(name, value, boost::make_shared<NullValidator>(), direction);
      declareProperty(p);
    }

    /** Specialised version of declareProperty template method to prevent the creation of a
    *  PropertyWithValue of type const char* if an argument in quotes is passed (it will be
    *  converted to a string). The validator, if provided, needs to be a string validator.
    *  @param name :: The name to assign to the property
    *  @param value :: The initial value to assign to the property
    *  @param validator :: Pointer to the (optional) validator. Ownership will be taken over.
    *  @param doc :: The (optional) documentation string
    *  @param direction :: The (optional) direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    void declareProperty( const std::string &name, const char* value,
                          IValidator_sptr validator = IValidator_sptr(new NullValidator), 
                          const std::string &doc="", const unsigned int direction = Direction::Input )
    {
        // Simply call templated method, converting character array to a string
        declareProperty(name, std::string(value), validator, doc, direction);
    }

    /** Specialised version of declareProperty template method to prevent the creation of a
    *  PropertyWithValue of type const char* if an argument in quotes is passed (it will be
    *  converted to a string). The validator, if provided, needs to be a string validator.
    *  @param name :: The name to assign to the property
    *  @param value :: The initial value to assign to the property
    *  @param doc :: The (optional) documentation string
    *  @param validator :: Pointer to the (optional) validator. Ownership will be taken over.
    *  @param direction :: The (optional) direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    void declareProperty( const std::string &name, const char* value,
                          const std::string &doc, IValidator_sptr validator = IValidator_sptr(new NullValidator), 
                          const unsigned int direction = Direction::Input )
    {
        // Simply call templated method, converting character array to a string
        declareProperty(name, std::string(value), validator, doc, direction);
    }

   /** Add a property of string type to the list of managed properties
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param direction :: The direction of the property, in, out or inout
   *  @throw Exception::ExistsError if a property with the given name already exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
    void declareProperty(const std::string &name, const char* value, const unsigned int direction)
    {
      declareProperty(name, std::string(value), boost::make_shared<NullValidator>(), "", direction);
    }


  /// Get a property by an index
  virtual Property* getPointerToPropertyOrdinal(const int &index) const = 0;

  /** Templated method to get the value of a property.
   *  No generic definition, only specialised ones. Although the definitions are mostly the
   *  same, Visual Studio can't cope with 
   *  @param name :: The name of the property (case insensitive)
   *  @return The value of the property
   *  @throw std::runtime_error If an attempt is made to assign a property to a different type
   *  @throw Exception::NotFoundError If the property requested does not exist
   */
  template<typename T>
  T getValue(const std::string &name) const;
  
  /// Clears all properties under management
  virtual void clear() = 0;
  /// Override this method to perform a custom action right after a property was set.
  /// The argument is the property name. Default - do nothing.
  virtual void afterPropertySet(const std::string&) {}

  /// Utility class that enables the getProperty() method to effectively be templated on the return type
  struct MANTID_KERNEL_DLL TypedValue
  {
    /// Reference to the containing PropertyManager
    const IPropertyManager& pm;
    /// The name of the property desired
    const std::string prop;

    /// Constructor
    TypedValue(const IPropertyManager& p, const std::string &name) : pm(p), prop(name) {}

    // Unfortunately, MSVS2010 can't handle just having a single templated cast operator T()
    // (it has problems with the string one). This operator is needed to convert a TypedValue
    // into what we actually want. It can't even handle just having a specialization for strings.
    // So we have to explicitly define an operator for each type of property that we have.

    operator int16_t ();
    operator uint16_t ();
    operator int32_t ();
    operator uint32_t ();
    operator int64_t ();
    operator uint64_t ();
  #ifdef __APPLE__
    operator unsigned long ();
  #endif
    /// explicit specialization for bool()
    operator bool ();
    /// explicit specialization for double()
    operator double ();
    /// explicit specialization for std::string()
    operator std::string ();
    /// explicit specialization for Property*()
    operator Property* ();
    
    /// explicit specialization for std::vector
    template<typename T>
    operator std::vector<T> () { return pm.getValue<std::vector<T> >(prop);}
    /// explicit specialization for std::vector
    template<typename T>
    operator std::vector<std::vector<T> > () { return pm.getValue<std::vector<std::vector<T> > >(prop);}
    /// explicit specialization for boost::shared_ptr
    template<typename T>
    operator boost::shared_ptr<T> () { return pm.getValue<boost::shared_ptr<T> >(prop); }
    /// explicit version for Matrices
    template<typename T>
    operator Matrix<T> () { return pm.getValue<Matrix<T> >(prop); }
  };

public:
  /// Get the value of a property
  virtual TypedValue getProperty(const std::string &name) const = 0;
  /// Get a pointer to property by name
  virtual Property* getPointerToProperty(const std::string &name) const = 0;

private:
  /**
   * Set a property value that is not convertible to a DataItem_sptr
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
   */
  template<typename T>
  IPropertyManager* setTypedProperty(const std::string &name, const T & value, const boost::false_type &)
  {
    PropertyWithValue<T> *prop = dynamic_cast<PropertyWithValue<T>*>(getPointerToProperty(name));
    if (prop)
    {
      *prop = value;
    }
    else
    {
      throw std::invalid_argument("Attempt to assign to property (" + name + ") of incorrect type");
    }
    return this;
  }
  /**
   * Set a property value that is convertible to a DataItem_sptr
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
   */
  template<typename T>
  IPropertyManager* setTypedProperty(const std::string &name, const T & value, const boost::true_type &)
  {
    // T is convertible to DataItem_sptr
    boost::shared_ptr<DataItem> data = boost::static_pointer_cast<DataItem>(value);
    std::string error = getPointerToProperty(name)->setDataItem(data);
    if( !error.empty() )
    {
      throw std::invalid_argument(error);
    }
    return this;
  }
  

};

} // namespace Kernel
} // namespace Mantid

/// A macro for defining getValue functions for new types. Puts them in the Mantid::Kernel namespace so
/// the macro should be used outside of any namespace scope
#define DEFINE_IPROPERTYMANAGER_GETVALUE(type) \
    namespace Mantid \
    { \
      namespace Kernel \
      { \
        template<> DLLExport \
        type IPropertyManager::getValue<type>(const std::string &name) const \
        { \
          PropertyWithValue<type> *prop = dynamic_cast<PropertyWithValue<type>*>(getPointerToProperty(name)); \
          if (prop) \
          { \
            return *prop; \
          } \
          else \
          { \
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected type "#type; \
            throw std::runtime_error(message); \
          } \
        } \
      } \
    }

#endif /*MANTID_KERNEL_IPROPERTYMANAGER_H_*/
