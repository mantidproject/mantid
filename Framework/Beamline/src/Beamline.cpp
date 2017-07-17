#include "MantidBeamline/Beamline.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"

namespace Mantid {
namespace Beamline {

Beamline::Beamline(std::unique_ptr<ComponentInfo> &&componentInfo,
                   std::unique_ptr<DetectorInfo> &&detectorInfo)
    : m_componentInfo(std::move(componentInfo)),
      m_detectorInfo(std::move(detectorInfo)) {
  m_componentInfo->setDetectorInfo(m_detectorInfo.get());
  m_detectorInfo->setComponentInfo(m_componentInfo.get());
}

const ComponentInfo &Beamline::componentInfo() const {
  return *m_componentInfo;
}

const DetectorInfo &Beamline::detectorInfo() const { return *m_detectorInfo; }

ComponentInfo &Beamline::mutableComponentInfo() { return *m_componentInfo; }

DetectorInfo &Beamline::mutableDetectorInfo() { return *m_detectorInfo; }
} // namespace Beamline
} // namespace Mantid
