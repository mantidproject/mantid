#include "MantidBeamline/Beamline.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include <memory>

namespace Mantid {
namespace Beamline {

namespace {

template <typename T>
std::unique_ptr<T> copyOrNull(const std::unique_ptr<T> &source) {
  return source ? std::make_unique<T>(*source) : std::unique_ptr<T>(nullptr);
}
}

Beamline::Beamline() :
    m_componentInfo(std::unique_ptr<ComponentInfo>(nullptr)),
    m_detectorInfo(std::unique_ptr<DetectorInfo>(nullptr))
{

}

/**
 * Copy constructor.
 *
 * Behavior here is really important to ensure that cow is acheived
 * properly for internals of held ComponentInfos and DetectorInfos
 *
 * @param other
 */
Beamline::Beamline(const Beamline &other)
    : m_empty(other.m_empty),
      m_componentInfo(copyOrNull(other.m_componentInfo)),
      m_detectorInfo(copyOrNull(other.m_detectorInfo)) {}

Beamline &Beamline::operator=(const Beamline &other) {
  if (&other != this) {
    m_componentInfo = copyOrNull(other.m_componentInfo);
    m_detectorInfo = copyOrNull(other.m_detectorInfo);
    m_empty = other.m_empty;
  }
  return *this;
}

Beamline::Beamline(ComponentInfo &&componentInfo, DetectorInfo &&detectorInfo)
    : m_empty(false),
      m_componentInfo(std::make_unique<ComponentInfo>(componentInfo)),
      m_detectorInfo(std::make_unique<DetectorInfo>(detectorInfo)) {
  m_componentInfo->setDetectorInfo(m_detectorInfo.get());
  m_detectorInfo->setComponentInfo(m_componentInfo.get());
}

const ComponentInfo &Beamline::componentInfo() const {
  return *m_componentInfo;
}

const DetectorInfo &Beamline::detectorInfo() const { return *m_detectorInfo; }

ComponentInfo &Beamline::mutableComponentInfo() { return *m_componentInfo; }

DetectorInfo &Beamline::mutableDetectorInfo() { return *m_detectorInfo; }

bool Beamline::empty() const { return m_empty; }
} // namespace Beamline
} // namespace Mantid
