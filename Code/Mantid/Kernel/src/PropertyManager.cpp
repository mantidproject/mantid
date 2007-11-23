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
  for ( std::vector<Property*>::const_iterator it = m_properties.begin() ; it != m_properties.end() ; ++it )
  {
    delete (*it);
  }
}

/** Add a property to the list of managed properties
 *  @param p The property object to add
 *  @throw ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( Property *p )
{
  if ( ! existsProperty( p->name() ) )
  {
    m_properties.push_back(p);
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
 *  @throw ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( const std::string &name, int value, const std::string &doc )
{
  if ( ! existsProperty( name ) )
  {
    Property *p = new PropertyWithValue<int>(name, value);
    p->setDocumentation(doc);
    m_properties.push_back(p);
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
 *  @throw ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( const std::string &name, double value, const std::string &doc )
{
  if ( ! existsProperty( name ) )
  {
    Property *p = new PropertyWithValue<double>(name, value);
    p->setDocumentation(doc);
    m_properties.push_back(p);
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
 *  @throw ExistsError if a property with the given name already exists
 */
void PropertyManager::declareProperty( const std::string &name, std::string value, const std::string &doc )
{
  if ( ! existsProperty( name ) )
  {
    Property *p = new PropertyWithValue<std::string>(name, value);
    p->setDocumentation(doc);
    m_properties.push_back(p);
  }
  else
  {
    throw Exception::ExistsError("Property with given name already exists", name );
} 
}

/** Set the value of a property by string
 *  @param name The name of the property (case insensitive)
 *  @param value The value to assign to the property
 *  @throw NotFoundError if the named property is unknown
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
  try {
    getProperty(name);
    return true;
  } catch (Exception::NotFoundError e) {
    return false;
  }
}

/** Get the value of a property as a string
 *  @param name The name of the property (case insensitive)
 *  @return The value of the named property
 *  @throw NotFoundError if the named property is unknown
 */
std::string PropertyManager::getPropertyValue( const std::string &name ) const
{
  Property *p = getProperty(name);   // throws NotFoundError if property not in vector
  return p->value();
}

/** Get a property by name
 *  @param name The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw NotFoundError if the named property is unknown
 */
Property* PropertyManager::getProperty( std::string name ) const
{
  /// TODO Re-work to be more efficient
  for ( std::vector<Property*>::const_iterator it = m_properties.begin() ; it != m_properties.end() ; ++it )
  {
    std::string n = (*it)->name();
    std::transform(n.begin(), n.end(), n.begin(), toupper);
    std::transform(name.begin(), name.end(), name.begin(), toupper);
    if ( ! name.compare(n) )
    {
      return *it;
    }
  }
  throw Exception::NotFoundError("Unable to retrieve property", name);
}

/** Get the list of managed properties
 *  @return A vector holding pointers to the list of properties
 */
const std::vector< Property* >& PropertyManager::getProperties() const
{
  return m_properties;
}

} // namespace Kernel
} // namespace Mantid
