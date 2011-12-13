#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"

namespace Mantid
{
namespace Geometry
{

MDPlaneImplicitFunction::MDPlaneImplicitFunction() : MDImplicitFunction()
{
}

MDPlaneImplicitFunction::~MDPlaneImplicitFunction()
{
}

void MDPlaneImplicitFunction::addPlane(const MDPlane &plane)
{
  if (this->getNumPlanes() > 0)
  {
    throw std::runtime_error("Only one plane per MDPlaneImplicitFunction.");
  }
  else
  {
    MDImplicitFunction::addPlane(plane);
  }
}

std::string MDPlaneImplicitFunction::getName() const
{
  return std::string("PlaneImplicitFuction");
}

std::string MDPlaneImplicitFunction::toXMLString() const
{
  return std::string("");
}

} // namespace Geometry
} // namespace Mantid
