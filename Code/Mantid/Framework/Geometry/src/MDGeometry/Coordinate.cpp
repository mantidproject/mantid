#include "MantidGeometry/MDGeometry/Coordinate.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

namespace Mantid
{
namespace Geometry
{
  ///Construction Method for 1D.
  Coordinate Coordinate::createCoordinate1D(const coord_t& xArg)
  {
    return Coordinate(xArg);
  }

  ///Construction Method for 2D.
  Coordinate Coordinate::createCoordinate2D(const coord_t& xArg, const coord_t& yArg)
  {
    return Coordinate(xArg, yArg);
  }

  ///Construction Method for 3D.
  Coordinate Coordinate::createCoordinate3D(const coord_t& xArg, const coord_t& yArg, const coord_t& zArg)
  {
    return Coordinate(xArg, yArg, zArg);
  }

  ///Construction Method for 4D.
  Coordinate Coordinate::createCoordinate4D(const coord_t& xArg, const coord_t& yArg,const coord_t& zArg,const coord_t& tArg)
  {
    return Coordinate(xArg, yArg, zArg, tArg);
  }

  /** Default Constructor */
  Coordinate::Coordinate()
  : m_x(0),m_y(0),m_z(0),m_t(0)
  {
  }

  /** Constructor
   *
   * @param coords :: array of Coordinates
   * @param numdims :: number of dimensions in array
   */
  Coordinate::Coordinate(Mantid::coord_t * coords, size_t numdims)
  : m_x(0),m_y(0),m_z(0),m_t(0)
  {
    if (numdims > 0) m_x = coords[0];
    if (numdims > 1) m_y = coords[1];
    if (numdims > 2) m_z = coords[2];
    if (numdims > 3) m_t = coords[3];
  }


  Coordinate::Coordinate(const coord_t& xArg)
  : m_x(xArg),m_y(0),m_z(0),m_t(0)
  {
  }

  Coordinate::Coordinate(const coord_t& xArg, const coord_t& yArg)
  : m_x(xArg),m_y(yArg),m_z(0),m_t(0)
  {
  }

  Coordinate::Coordinate(const coord_t& xArg, const coord_t& yArg, const coord_t& zArg)
  : m_x(xArg),m_y(yArg),m_z(zArg),m_t(0)
  {
  }

  Coordinate::Coordinate(const coord_t& xArg, const coord_t& yArg,const coord_t& zArg,const coord_t& tArg)
  : m_x(xArg),m_y(yArg),m_z(zArg),m_t(tArg)
  {
  }

  Coordinate::Coordinate(const Coordinate & other)
  : m_x(other.m_x),m_y(other.m_y),m_z(other.m_z),m_t(other.m_t)
  {
  }

  Coordinate & Coordinate::operator= (const Coordinate & other)
  {
    if(this != &other)
    {
    m_x = other.m_x;
    m_y = other.m_y;
    m_z = other.m_z;
    m_t = other.m_t;
    }
    return *this;
  }

  coord_t Coordinate::getX() const
  {
    return m_x;
  }
  coord_t Coordinate::getY() const
  {
    return m_y;
  }
  coord_t Coordinate::getZ() const
  {
    return m_z;
  }
  coord_t Coordinate::gett() const
  {
    return m_t;
  }
}
}
