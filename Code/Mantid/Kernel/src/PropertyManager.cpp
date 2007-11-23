//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Exception.h"
#include <algorithm>

namespace Mantid
{
namespace Kernel
{

/// Default constructor
PropertyManager::PropertyManager() :
  m_properties()
{
}

/// Virtual destructor
PropertyManager::~PropertyManager()
{
  for ( PropertyMap::iterator it = m_properties.begin(); it != m_properties.end(); ++it )
  {
    delete it->second;
  }
}

/** Add a property to the list of managed properties
 *  @param p The property object to add
 *  @throw Exception::ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( Property *p )
{
  std::string key = p->name();
  std::transform(key.begin(), key.end(), key.begin(), toupper);
  if ( m_properties.insert(PropertyMap::value_type(key, p)).second)
  {
    m_orderedNames.push_back(key);    
  }
  else
  {
    throw Exception::ExistsError("Property with given name already exists", p->name() );
  }
}

/** Add an integer property to the list of managed properties
 *  @param name The name to assign to the property
 *  @param value The initial value to assign to the property
 *  @param doc The (optional) documentation string
 *  @throw Exception::ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( const std::string &name, int value, const std::string &doc )
{
  std::string key = name;
  std::transform(key.begin(), key.end(), key.begin(), toupper);
  if ( ! existsProperty( key ) )
  {
    Property *p = new PropertyWithValue<int>(name, value);
    p->setDocumentation(doc);
    m_properties.insert(PropertyMap::value_type(key, p));
    m_orderedNames.push_back(key);
  }
  else
  {
    throw Exception::ExistsError("Property with given name already exists", name );
  }
}

/** Add a double property to the list of managed properties
 *  @param name The name to assign to the property
 *  @param value The initial value to assign to the property
 *  @param doc The (optional) documentation string
 *  @throw Exception::ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( const std::string &name, double value, const std::string &doc )
{
  std::string key = name;
  std::transform(key.begin(), key.end(), key.begin(), toupper);
  if ( ! existsProperty( key ) )
  {
    Property *p = new PropertyWithValue<double>(name, value);
    p->setDocumentation(doc);
    m_properties.insert(PropertyMap::value_type(key, p));
    m_orderedNames.push_back(key);
  }
  else
  {
    throw Exception::ExistsError("Property with given name already exists", name );
  }
}

/** Add a string property to the list of managed properties
 *  @param name The name to assign to the property
 *  @param value The initial value to assign to the property
 *  @param doc The (optional) documentation string
 *  @throw Exception::ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( const std::string &name, std::string value, const std::string &doc )
{
  std::string key = name;
  std::transform(key.begin(), key.end(), key.begin(), toupper);
  if ( ! existsProperty( key ) )
  {
    Property *p = new PropertyWithValue<std::string>(name, value);
    p->setDocumentation(doc);
    m_properties.insert(PropertyMap::value_type(key, p));
    m_orderedNames.push_back(key);
  }
  else
  {
    throw Exception::ExistsError("Property with given name already exists", name );
  }
}

/** Set the ordered list of properties by one string of values.
 *  @param values The list of property values
 *  @throws Exception::NotImplementedError because it isn't, yet
 */
// Care will certainly be required in the calling of this function or it could all go horribly wrong!
void PropertyManager::setProperties( const std::string &values )
{
  throw Exception::NotImplementedError("Coming to an iteration near you soon...");
}

/** Set the value of a property by string
 *  @param name The name of the property (case insensitive)
 *  @param value The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
void PropertyManager::setProperty( const std::string &name, const std::string &value )
{
  Property *p = getProperty(name);   // throws NotFoundError if property not in vector
  bool success = p->setValue(value);
  if ( !success ) throw std::invalid_argument("Invalid value for this property");
}

/** Checks whether the named property is already in the list of managed property.
 *  @param name The name of the property (case insensitive)
 *  @return True if the property is already stored
 */
bool PropertyManager::existsProperty( const std::string& name ) const
{
  std::string ucName = name;
  std::transform(ucName.begin(), ucName.end(), ucName.begin(), toupper);
  PropertyMap::const_iterator it = m_properties.find(ucName);
  if (it != m_properties.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

/** Get the value of a property as a string
 *  @param name The name of the property (case insensitive)
 *  @return The value of the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
std::string PropertyManager::getPropertyValue( const std::string &name ) const
{
  Property *p = getProperty(name);   // throws NotFoundError if property not in vector
  return p->value();
}

/** Get a property by name
 *  @param name The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
Property* PropertyManager::getProperty( const std::string &name ) const
{
  std::string ucName = name;
  std::transform(ucName.begin(), ucName.end(), ucName.begin(), toupper);
  PropertyMap::const_iterator it = m_properties.find(ucName);
  if (it != m_properties.end())
  {
    return it->second;
  }
  throw Exception::NotFoundError("Unknown property", name);
}

/** Get the list of managed properties.
 *  The properties will be stored in the order that they were declared.
 *  @return A vector holding pointers to the list of properties
 */
std::vector< Property* > PropertyManager::getProperties() const
{
  std::vector< Property* > p;
  for (std::vector<std::string>::const_iterator it = m_orderedNames.begin(); it != m_orderedNames.end(); ++it)
  {
    p.push_back( getProperty(*it) );
  }
  return p;
}

} // namespace Kernel
} // namespace Mantid
