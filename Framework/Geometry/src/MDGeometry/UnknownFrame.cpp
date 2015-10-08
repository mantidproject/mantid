#include "MantidGeometry/MDGeometry/UnknownFrame.h"

namespace Mantid {
namespace Geometry {

UnknownFrame::UnknownFrame(const std::string &frameName,
                           std::unique_ptr<Kernel::MDUnit> unit)
    : GeneralFrame(frameName, std::move(unit)) {}

UnknownFrame::UnknownFrame(const std::string &frameName,
                           const Kernel::UnitLabel &unit)
    : GeneralFrame(frameName, unit) {}

UnknownFrame::~UnknownFrame() {}

const std::string UnknownFrame::UnknownFrameName = "Unknown frame";
}
}
