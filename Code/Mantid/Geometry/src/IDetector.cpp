#include "MantidGeometry/IDetector.h"

using namespace Mantid::Geometry;

/// Must return a pointer to itself if derived from IComponent
IComponent* IDetector::getComponent()
{
  throw std::runtime_error("This detector class does not inherit from IComponent.");
}

/// Default implementation
std::vector<double> IDetector::getNumberParameter(const std::string &) const
{
  return std::vector<double>(0);
} 

/// Default implementation
std::vector<V3D> IDetector::getPositionParameter(const std::string &) const
{
  return std::vector<V3D>(0);
} 

/// Default implementation
std::vector<Quat> IDetector::getRotationParameter(const std::string &) const
{
  return std::vector<Quat>(0);
} 
