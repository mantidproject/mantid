#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace API {

DetectorInfo::DetectorInfo(const Geometry::Instrument &instrument)
    : m_detectorIDs(
          instrument.getDetectorIDs(false /* do not skip monitors */)) {}

const std::vector<detid_t> DetectorInfo::detectorIDs() const {
  return m_detectorIDs;
}

} // namespace API
} // namespace Mantid
