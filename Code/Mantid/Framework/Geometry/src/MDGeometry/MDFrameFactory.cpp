#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"

namespace Mantid {
namespace Geometry {

GeneralFrame *
GeneralFrameFactory::createRaw(const std::string &argument) const{
    return new GeneralFrame(argument, argument); // HACK
}

/// Indicate an ability to intepret the string
bool GeneralFrameFactory::canInterpret(const std::string &) const{
    return true;
}

} // namespace Geometry
} // namespace Mantid
