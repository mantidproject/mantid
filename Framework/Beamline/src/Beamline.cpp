#include "MantidBeamline/Beamline.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Beamline {

namespace {

template <typename T>
boost::shared_ptr<T> copyOrNull(const boost::shared_ptr<T> &source) {
  return source ? boost::make_shared<T>(*source)
                : boost::shared_ptr<T>(nullptr);
}

bool bindTogether(ComponentInfo *compInfo, DetectorInfo *detInfo) {
  bool canBind = compInfo != nullptr && detInfo != nullptr;
  if (canBind) {
    compInfo->setDetectorInfo(detInfo);
    detInfo->setComponentInfo(compInfo);
  }
  return canBind;
}
}

Beamline::Beamline()
    : m_empty(true), m_componentInfo(boost::shared_ptr<ComponentInfo>(nullptr)),
      m_detectorInfo(boost::shared_ptr<DetectorInfo>(nullptr)) {}

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
      m_detectorInfo(copyOrNull(other.m_detectorInfo)) {
  bindTogether(m_componentInfo.get(), m_detectorInfo.get());
}

Beamline &Beamline::operator=(const Beamline &other) {
  if (&other != this) {
    m_componentInfo = copyOrNull(other.m_componentInfo);
    m_detectorInfo = copyOrNull(other.m_detectorInfo);
    m_empty = other.m_empty;
    bindTogether(m_componentInfo.get(), m_detectorInfo.get());
  }
  return *this;
}

Beamline::Beamline(ComponentInfo &&componentInfo, DetectorInfo &&detectorInfo)
    : m_empty(false),
      m_componentInfo(boost::make_shared<ComponentInfo>(componentInfo)),
      m_detectorInfo(boost::make_shared<DetectorInfo>(detectorInfo)) {
  bindTogether(m_componentInfo.get(), m_detectorInfo.get());
}

Beamline::Beamline(boost::shared_ptr<ComponentInfo> &componentInfo,
                   boost::shared_ptr<DetectorInfo> &detectorInfo)
    : m_empty(false), m_componentInfo(componentInfo),
      m_detectorInfo(detectorInfo) {
  // Code using this should already have these two objects, bound. But we ensure
  // it.
  bindTogether(m_componentInfo.get(), m_detectorInfo.get());
}

const ComponentInfo &Beamline::componentInfo() const {
  return *m_componentInfo;
}

const DetectorInfo &Beamline::detectorInfo() const { return *m_detectorInfo; }

ComponentInfo &Beamline::mutableComponentInfo() { return *m_componentInfo; }

DetectorInfo &Beamline::mutableDetectorInfo() { return *m_detectorInfo; }

/**
 * Create an alias of this beamline.
 *
 * Internally this increments the shared_ptr counts, so (1)
 * the same underling ComponentInfo and DetectorInfo objects are used.
 * so the Beamline from which the alias was created can die and alias
 * is still consistent, as required for Instruments shared pointers used after
 * ExperimentInfos go out of scope.
 *
 * (2) The ComponentInfo and DetectorInfo objects are the same, so, Components
 *accessed via
 * Parameter maps (i.e. Detector::getPos(ID) ) use the same ComponentInfo and
 *DetectorInfo
 * as exists on the ExperimentInfo. This is essential for DetectorInfo merging.
 * @return
 */
Beamline Beamline::alias() { return Beamline(m_componentInfo, m_detectorInfo); }

bool Beamline::empty() const { return m_empty; }

} // namespace Beamline
} // namespace Mantid
