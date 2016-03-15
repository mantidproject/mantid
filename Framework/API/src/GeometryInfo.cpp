#include "MantidAPI/GeometryInfo.h"

#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid {
namespace API {

GeometryInfo::GeometryInfo(const GeometryInfoFactory &factory,
                           const ISpectrum &spectrum)
    : m_factory(factory) {
  // Note: This constructor body has big overlap with the method
  // MatrixWorkspace::getDetector(). The plan is to eventually remove the
  // latter, once GeometryInfo is in widespread use.
  const std::set<detid_t> &dets = spectrum.getDetectorIDs();

  const size_t ndets = dets.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    m_detector = m_factory.getInstrument().getDetector(*dets.begin());
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           "");
  } else {
    // Else need to construct a DetectorGroup and use that
    std::vector<Geometry::IDetector_const_sptr> dets_ptr =
        m_factory.getInstrument().getDetectors(dets);
    m_detector = Geometry::IDetector_const_sptr(
        new Geometry::DetectorGroup(dets_ptr, false));
  }
}

bool GeometryInfo::isMonitor() const { return m_detector->isMonitor(); }

bool GeometryInfo::isMasked() const { return m_detector->isMasked(); }

double GeometryInfo::getL1() const { return m_factory.getL1(); }

double GeometryInfo::getL2() const {

  if (!m_l2) {
    if (!isMonitor()) {
      auto &sample = m_factory.getSample();
      std::lock_guard<std::mutex> lock(m_l2Lock);
      if (!m_l2) {
        m_l2 = m_detector->getDistance(sample);
      }
    } else {
      auto &source = m_factory.getSource();
      std::lock_guard<std::mutex> lock(m_l2Lock);
      if (!m_l2) {
        m_l2 = m_detector->getDistance(source) - getL1();
      }
    }
  }
  return *m_l2;
}

void GeometryInfo::invalidateCache() {
  std::lock_guard<std::mutex> lock_l2(m_l2Lock);
  m_l2 = boost::optional<double>{};
  std::lock_guard<std::mutex> lock_twoTheta(m_twoThetaLock);
  m_twoTheta = boost::optional<double>{};
  std::lock_guard<std::mutex> lock_signedTwoTheta(m_signedTwoThetaLock);
  m_signedTwoTheta = boost::optional<double>{};
}

GeometryInfo::GeometryInfo(GeometryInfo &&original)
    : m_factory(original.m_factory) {
  this->m_l2 = std::move(original.m_l2);
  this->m_twoTheta = std::move(original.m_twoTheta);
  this->m_signedTwoTheta = std::move(original.m_signedTwoTheta);
}

GeometryInfo &GeometryInfo::operator=(GeometryInfo &&original) {
  this->m_l2 = std::move(original.m_l2);
  this->m_twoTheta = std::move(original.m_twoTheta);
  this->m_signedTwoTheta = std::move(original.m_signedTwoTheta);
  return *this;
}

double GeometryInfo::getTwoTheta() const {

  if (!m_twoTheta) {
    // Note: This function has big overlap with the method
    // MatrixWorkspace::detectorTwoTheta(). The plan is to eventually remove the
    // latter, once GeometryInfo is in widespread use.
    const Kernel::V3D samplePos = m_factory.getSamplePos();
    const Kernel::V3D beamLine = samplePos - m_factory.getSourcePos();

    if (beamLine.nullVector()) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "Source and sample are at same position!");
    }

    // TODO. According to ConvertUnits. two-theta is always set to zero for
    // monitors.
    std::lock_guard<std::mutex> lock(m_twoThetaLock);
    if (!m_twoTheta) {
      m_twoTheta = m_detector->getTwoTheta(samplePos, beamLine);
    }
  }
  return *m_twoTheta;
}

double GeometryInfo::getSignedTwoTheta() const {

  if (!m_signedTwoTheta) {
    // Note: This function has big overlap with the method
    // MatrixWorkspace::detectorSignedTwoTheta(). The plan is to eventually
    // remove
    // the latter, once GeometryInfo is in widespread use.
    const Kernel::V3D samplePos = m_factory.getSamplePos();
    const Kernel::V3D beamLine = samplePos - m_factory.getSourcePos();

    if (beamLine.nullVector()) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "Source and sample are at same position!");
    }
    // Get the instrument up axis.
    const Kernel::V3D &instrumentUpAxis =
        m_factory.getInstrument().getReferenceFrame()->vecPointingUp();

    std::lock_guard<std::mutex> lock(m_signedTwoThetaLock);
    if (!m_signedTwoTheta) {
      m_signedTwoTheta =
          m_detector->getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
    }
  }
  return *m_signedTwoTheta;
}

boost::shared_ptr<const Geometry::IDetector> GeometryInfo::getDetector() const {
  return m_detector;
}
}
}
