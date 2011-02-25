#include "MantidGeometry/MDGeometry/Coordinate.h"

namespace Mantid
{
namespace Geometry
{
///Construction Method for 1D.
  coordinate coordinate::createCoordinate1D(const double& xArg)
  {
    return coordinate(xArg);
  }

  ///Construction Method for 2D.
  coordinate coordinate::createCoordinate2D(const double& xArg, const double& yArg)
  {
    return coordinate(xArg, yArg);
  }

  ///Construction Method for 3D.
  coordinate coordinate::createCoordinate3D(const double& xArg, const double& yArg, const double& zArg)
  {
    return coordinate(xArg, yArg, zArg);
  }

  ///Construction Method for 4D.
  coordinate coordinate::createCoordinate4D(const double& xArg, const double& yArg,const double& zArg,const double& tArg)
  {
    return coordinate(xArg, yArg, zArg, tArg);
  }

  coordinate::coordinate()
  {
    m_x = 0;
    m_y = 0;
    m_z = 0;
    m_t = 0;
  }

  coordinate::coordinate(const double& xArg)
  {
    m_x = xArg;
    m_y = 0;
    m_z = 0;
    m_t = 0;
  }

  coordinate::coordinate(const double& xArg, const double& yArg)
  {
    m_x = xArg;
    m_y = yArg;
    m_z = 0;
    m_t = 0;
  }

  coordinate::coordinate(const double& xArg, const double& yArg, const double& zArg)
  {
    m_x = xArg;
    m_y = yArg;
    m_z = zArg;
    m_t = 0;
  }

  coordinate::coordinate(const double& xArg, const double& yArg,const double& zArg,const double& tArg)
  {
    m_x = xArg;
    m_y = yArg;
    m_z = zArg;
    m_t = tArg;
  }

  coordinate::coordinate(const coordinate & other)
  {
    m_x = other.m_x;
    m_y = other.m_y;
    m_z = other.m_z;
    m_t = other.m_t;
  }

  coordinate & coordinate::operator= (const coordinate & other)
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

  double coordinate::getX() const
  {
    return m_x;
  }
  double coordinate::getY() const
  {
    return m_y;
  }
  double coordinate::getZ() const
  {
    return m_z;
  }
  double coordinate::gett() const
  {
    return m_t;
  }
}
}
