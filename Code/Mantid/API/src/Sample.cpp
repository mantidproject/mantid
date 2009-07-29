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
  m_name(), m_manager(), m_protonCharge(0.0), m_sample_shape(), m_geom_id(0), m_thick(0.0),
  m_height(0.0), m_width(0.0)
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
void Sample::setShapeObject(boost::shared_ptr<Geometry::Object> sample_shape)
{
  m_sample_shape = sample_shape;
}

/**
 * Get a pointer to the sample shape object
 * @returns A shared pointer to the sample object
 */
boost::shared_ptr<Geometry::Object> Sample::getShapeObject() const
{
  return m_sample_shape;
}

/**
 * Set the geometry flag that is specfied in the raw file within the SPB_STRUCT
 * 1 = cylinder, 2 = flat plate, 3 = disc, 4 = single crystal
 * @param geom_id The flag for the geometry
 */
void Sample::setGeometryFlag(int geom_id)
{
  m_geom_id = geom_id;
}

/**
 * Get the geometry flag that is specified in the raw file within the SPB_STRUCT
 * 1 = cylinder, 2 = flat plate, 3 = disc, 4 = single crystal
 * @returns The flag for the sample geometry
 */
int Sample::getGeometryFlag() const
{
  return m_geom_id;
}

/**
 * Set the thickness, width and height values that are pecified in the raw file.
 * @param thick The parameter e_thick in the SPB_STRUCT
 * @param height The parameter e_height in the SPB_STRUCT
 * @param width The parameter e_width in the SPB_STRUCT
 */
void Sample::setGeometry(double thick, double height, double width)
{
  m_thick = thick;
  m_height = height;
  m_width = width;
}

/**
 * Get the thickness, width and height values that are specified in the raw file.
 * @param thick An output parameter for e_thick in the SPB_STRUCT
 * @param height An output parameter for e_height in the SPB_STRUCT
 * @param width An output parameter for e_width in the SPB_STRUCT
 */
void Sample::getGeometry(double &thick, double &height, double &width) const
{
  thick = m_thick;
  height = m_height;
  width = m_width;
}


} // namespace API
} // namespace Mantid
