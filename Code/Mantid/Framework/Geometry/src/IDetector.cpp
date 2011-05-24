#include "MantidGeometry/IDetector.h"

using namespace Mantid::Geometry;

/// Must return a pointer to itself if derived from IComponent
IComponent* IDetector::getComponent()
{
  throw std::runtime_error("This detector class does not inherit from IComponent.");
}

