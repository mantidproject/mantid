//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Exception.h"
#include <algorithm>

namespace Mantid
{
  namespace Kernel
  {

    // Get a reference to the logger
    Logger& PropertyManagerOwner::g_log = Logger::get("PropertyManagerOwner");

    /// Default constructor
    PropertyManagerOwner::PropertyManagerOwner() :
    m_properties(new PropertyManager)
    {
    }

    /// Copy constructor
    PropertyManagerOwner::PropertyManagerOwner(const PropertyManagerOwner& po)
    {
        m_properties = po.m_properties;
    }

    /// Assignment operator
    PropertyManagerOwner& PropertyManagerOwner::operator=(const PropertyManagerOwner& po)
    {
        m_properties = po.m_properties;
        return *this;
    }

    /** Add a property to the list of managed properties
    *  @param p :: The property object to add
    *  @param doc :: A description of the property that may be displayed to users
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the property declared has an empty name.
    */
    void PropertyManagerOwner::declareProperty( Property *p, const std::string &doc )
    {
        m_properties->declareProperty(p, doc);
    }

    /** Set the ordered list of properties by one string of values.
    *  @param propertiesArray :: The list of property values
    *  @throw invalid_argument if error in parameters
    */
    // Care will certainly be required in the calling of this function or it could all go horribly wrong!
    void PropertyManagerOwner::setProperties( const std::string &propertiesArray )
    {
        m_properties->setProperties( propertiesArray );
    }

    /** Set the value of a property by string
    *  N.B. bool properties must be set using 1/0 rather than true/false
    *  @param name :: The name of the property (case insensitive)
    *  @param value :: The value to assign to the property
    *  @throw Exception::NotFoundError if the named property is unknown
    *  @throw std::invalid_argument If the value is not valid for the property given
    */
    void PropertyManagerOwner::setPropertyValue( const std::string &name, const std::string &value )
    {
        m_properties->setPropertyValue( name, value );
    }

    /** Set the value of a property by an index
    *  N.B. bool properties must be set using 1/0 rather than true/false
    *  @param index :: The index of the property to assign
    *  @param value :: The value to assign to the property
    *  @throw std::runtime_error if the property index is too high
    */
    void PropertyManagerOwner::setPropertyOrdinal( const int& index, const std::string &value )
    {
        m_properties->setPropertyOrdinal( index, value );
    }


    /** Checks whether the named property is already in the list of managed property.
    *  @param name :: The name of the property (case insensitive)
    *  @return True if the property is already stored
    */
    bool PropertyManagerOwner::existsProperty( const std::string& name ) const
    {
        return m_properties->existsProperty( name );
    }

    /** Validates all the properties in the collection
    *  @return True if all properties have a valid value
    */
    bool PropertyManagerOwner::validateProperties() const
    {
        return m_properties->validateProperties();
    }

    /**
     * Count the number of properties under management
     * @returns The number of properties being managed
     */
    size_t PropertyManagerOwner::propertyCount() const
    {
      return m_properties->propertyCount();
    }

    /** Get the value of a property as a string
    *  @param name :: The name of the property (case insensitive)
    *  @return The value of the named property
    *  @throw Exception::NotFoundError if the named property is unknown
    */
    std::string PropertyManagerOwner::getPropertyValue( const std::string &name ) const
    {
        return m_properties->getPropertyValue( name );
    }

    /** Get a property by name
    *  @param name :: The name of the property (case insensitive)
    *  @return A pointer to the named property
    *  @throw Exception::NotFoundError if the named property is unknown
    */
    Property* PropertyManagerOwner::getPointerToProperty( const std::string &name ) const
    {
        return m_properties->getPointerToProperty( name );
    }

    /** Get a property by an index
    *  @param index :: The name of the property (case insensitive)
    *  @return A pointer to the named property
    *  @throw std::runtime_error if the property index is too high
    */
    Property* PropertyManagerOwner::getPointerToPropertyOrdinal( const int& index) const
    {
        return m_properties->getPointerToPropertyOrdinal( index );
    }

    /** Get the list of managed properties.
    *  The properties will be stored in the order that they were declared.
    *  @return A vector holding pointers to the list of properties
    */
    const std::vector< Property* >& PropertyManagerOwner::getProperties() const
    {
      return m_properties->getProperties();
    }


    /** Get the value of a property. Allows you to assign directly to a variable of the property's type
    *  (if a supported type).
    *
    *  *** This method does NOT work for assigning to an existing std::string.
    *      In this case you have to use getPropertyValue() instead.
    *      Note that you can, though, construct a local string variable by writing,
    *      e.g. std::string s = getProperty("myProperty"). ***
    *
    *  @param name :: The name of the property
    *  @return The value of the property. Will be cast to the desired type (if a supported type).
    *  @throw std::runtime_error If an attempt is made to assign a property to a different type
    *  @throw Exception::NotFoundError If the property requested does not exist
    */
    IPropertyManager::TypedValue PropertyManagerOwner::getProperty( const std::string &name ) const
    {
        return m_properties->getProperty( name );
    }

    /**
    * Return the property manager serialized as a string.
    * The format is propName=value,propName=value,propName=value
    * @param withDefaultValues :: If true then the value of default parameters will be included
    * @returns A stringized version of the manager
    */
    std::string PropertyManagerOwner::toString(bool withDefaultValues) const
    {
      return m_properties->toString(withDefaultValues);
    }

    /** 
     * Removes the property from properties map.
     * @param name :: Name of the property to be removed.
     */
    void PropertyManagerOwner::removeProperty(const std::string &name)
    {
      m_properties->removeProperty(name);
    }

    /**
     * Clears all properties under management
     */
    void PropertyManagerOwner::clear()
    {
      m_properties->clear();
    }


  } // namespace Kernel
} // namespace Mantid
