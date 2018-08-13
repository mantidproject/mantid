#ifndef NEXUSGEOMETRYTESTHELPERS_H
#define NEXUSGEOMETRYTESTHELPERS_H

#include <boost/shared_ptr.hpp>
#include <vector>

#include "MantidNexusGeometry/TubeHelpers.h"

namespace Mantid {
namespace Geometry {
class IObject;
}
} // namespace Mantid

namespace NexusGeometryTestHelpers {
boost::shared_ptr<const Mantid::Geometry::IObject> createShape();
Pixels generateValidPixels();
Pixels generateInvalidPixels();
std::vector<int> getFakeDetIDs();
} // namespace NexusGeometryTestHelpers

#endif // NEXUSGEOMETRYTESTHELPERS_H