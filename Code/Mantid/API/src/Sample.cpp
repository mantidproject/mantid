//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Sample.h"

namespace Mantid
{
namespace API
{

/// Constructor
Sample::Sample() :
  m_manager()
{
}

/// Destructor
Sample::~Sample()
{
}

/** Set the name of the sample
 *  @param name The name to assign
 */
void Sample::setName( const std::string &name )
{
  m_name = name;
}

/** Gets the name of the sample
 *  @return The name of the sample
 */
const std::string& Sample::getName() const
{
  return m_name;
}

/** Add a set of data as read in from a log file.
 *  Note that the sample takes over ownership of the data, so the user should not delete it.
 *  @param p A pointer to the property (typically a TimeSeriesProperty) containing the data.
 */
void Sample::addLogData( Property *p )
{
  m_manager.declareProperty(p);
}

/** Retrieve a particular dataset
 *  @param name The name of the dataset (as contained in the original property object)
 *  @return A pointer to the property containing the dataset
 */
Property* Sample::getLogData( const std::string &name ) const
{
  return m_manager.getProperty(name);
}

/** Retrieve the full set of log properties
 *  @return A vector of pointers to the property objects
 */
const std::vector<Property*>& Sample::getLogData() const
{
  return m_manager.getProperties();
}

} // namespace API
} // namespace Mantid
