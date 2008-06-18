#include "MantidKernel/Property.h"

namespace Mantid
{
namespace Kernel
{

/** Constructor
 *  @param name The name of the property
 *  @param type The type of the property
 */
Property::Property( const std::string &name, const std::type_info &type ) :
  m_isDefault( true ),
  m_name( name ),
  m_documentation( "" ),
  m_typeinfo( &type )
{
}

/// Copy constructor
Property::Property( const Property& right ) :
  m_isDefault( right.m_isDefault ),
  m_name( right.m_name ),
  m_documentation( right.m_documentation ),
  m_typeinfo( right.m_typeinfo )
{
}

/// Virtual destructor
Property::~Property()
{
}

/** Copy assignment operator. Does nothing.
* @param right The right hand side value
*/
Property& Property::operator=( const Property& right )
{
  return *this;
}

/** Get the property's name
 *  @return The name of the property
 */
const std::string& Property::name() const
{
  return m_name;
}

/** Get the property's documentation string
 *  @return The documentation string
 */
const std::string& Property::documentation() const
{
  return m_documentation;
}

/** Get the property type_info
 *  @return The type of the property
 */
const std::type_info* Property::type_info() const
{
  return m_typeinfo;
}

/** Returns the type of the property as a string.
 *  Note that this is implementation dependent.
 *  @return The property type
 */
const std::string Property::type() const
{
  return m_typeinfo->name();
}

/** Checks whether the property has a valid value.
 *  Note: always returns true unless overridden
 *  @return True if the property's value is a valid one
 */
const bool Property::isValid() const
{
  // Always true at present
  return true;
}

/** Returns true if the property has not been changed since initialisation
 *  @return True if the property still has its default value
 */
const bool Property::isDefault() const
{
  return m_isDefault;
}

/** Sets the property's (optional) documentation string
 *  @param documentation The string containing the descriptive comment
 */
void Property::setDocumentation( const std::string& documentation )
{
  m_documentation = documentation;
}

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty set.
 */
const std::set<std::string> Property::allowedValues() const
{
  return std::set<std::string>();
}

} // namespace Kernel
} // namespace Mantid
