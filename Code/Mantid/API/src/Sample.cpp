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
/**  copy constructor 
  *  @param const reference to the sample object
*/
  Sample::Sample(const Sample& copy):m_name(copy.m_name),m_protonCharge(copy.m_protonCharge),
	  m_geom_id(copy.m_geom_id),m_thick(copy.m_thick),m_height(copy.m_height),m_width(copy.m_width),m_manager(copy.m_manager)
  {  

  }
/**  assignment operator 
  * @param const reference to the sample object
*/
  const Sample& Sample::operator=(const Sample&rhs)
  {
	  if(this==&rhs)return *this;
	  m_name=rhs.m_name;
	  m_protonCharge=rhs.m_protonCharge;
	  m_geom_id=rhs.m_geom_id;
	  m_thick=rhs.m_thick;
	  m_height=rhs.m_height;
	  m_width=rhs.m_width;
	 m_manager=rhs.m_manager;
	  return *this;
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
 * Set the thickness value
 * @param thick The parameter e_thick in the SPB_STRUCT
 */
void Sample::setThickness(double thick)
{
  m_thick = thick;
}

/**
 * Get the thickness value
 * @returns The parameter thickness parameter
 */
double Sample::getThickness() const
{
  return m_thick;
}

/**
 * Set the height value
 * @param height The parameter e_height in the SPB_STRUCT
 */
void Sample::setHeight(double height)
{
  m_height = height;
}

/**
 * Get the height value
 * @returns The parameter height parameter
 */
double Sample::getHeight() const
{
  return m_height;
}

/**
 * Set the width value
 * @param width The parameter e_width in the SPB_STRUCT
 */
void Sample::setWidth(double width)
{
  m_width = width;
}

/**
 * Get the height value
 * @returns The parameter height parameter
 */
double Sample::getWidth() const
{
  return m_width;
}

} // namespace API
} // namespace Mantid
