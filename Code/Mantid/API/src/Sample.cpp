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
  m_name(), m_manager(), m_protonCharge(0.0), m_sample_shape()
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
void Sample::addLogData( Kernel::Property *p )
{
  m_manager.declareProperty(p);
}

/** Retrieve a particular dataset
 *  @param name The name of the dataset (as contained in the original property object)
 *  @return A pointer to the property containing the dataset
 */
Kernel::Property* Sample::getLogData( const std::string &name ) const
{
  return m_manager.getProperty(name);
}

/** Retrieve the full set of log properties
 *  @return A vector of pointers to the property objects
 */
const std::vector<Kernel::Property*>& Sample::getLogData() const
{
  return m_manager.getProperties();
}

/** Set the good proton charge total for this run
 *  @param charge The proton charge in uA.hour
 */
void Sample::setProtonCharge( const double &charge)
{
  m_protonCharge = charge;
}

/** Retrieve the total good proton charge delivered in this run
 *  @return The proton charge in uA.hour
 */
const double& Sample::getProtonCharge() const
{
  return m_protonCharge;
}

/**
 * Set the object that describes the sample shape
 * @param sample_shape The shape object
 */
void Sample::setGeometry(boost::shared_ptr<Geometry::Object> sample_shape)
{
  m_sample_shape = sample_shape;
}

/**
 * Get a pointer to the sample shape object
 * @returns A shared pointer to the sample object
 */
boost::shared_ptr<Geometry::Object> Sample::getGeometry() const
{
  return m_sample_shape;
}


} // namespace API
} // namespace Mantid
