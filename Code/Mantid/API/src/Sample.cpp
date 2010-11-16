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
  m_name(), m_sample_shape(), m_geom_id(0), m_thick(0.0),
  m_height(0.0), m_width(0.0)
{
}

/**  copy constructor 
  *  @param copy const reference to the sample object
*/
Sample::Sample(const Sample& copy) :
  m_name(copy.m_name), m_sample_shape(copy.m_sample_shape), m_geom_id(copy.m_geom_id), 
  m_thick(copy.m_thick), m_height(copy.m_height), m_width(copy.m_width)
{
}

/**  assignment operator 
 * @param rhs const reference to the sample object
 * @return copy of sample object
 */
  const Sample& Sample::operator=(const Sample&rhs)
{
  if (this == &rhs) return *this;
  m_name = rhs.m_name;
  m_sample_shape = rhs.m_sample_shape;
  m_geom_id = rhs.m_geom_id;
  m_thick = rhs.m_thick;
  m_height = rhs.m_height;
  m_width = rhs.m_width;
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

/**
 * Set the object that describes the sample shape
 * @param sample_shape The shape object
 */
void Sample::setShapeObject(const Geometry::Object & sample_shape)
{
  m_sample_shape = sample_shape;
}

/**
 * Get a pointer to the sample shape object
 * @returns A shared pointer to the sample object
 */
const Geometry::Object& Sample::getShapeObject() const
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

}
}
