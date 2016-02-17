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
  if (!isMonitor()) {
    auto &sample = m_factory.getSample();
    return m_detector->getDistance(sample);
  } else {
    auto &source = m_factory.getSource();
    return m_detector->getDistance(source) - getL1();
  }
}

double GeometryInfo::getTwoTheta() const {
  // Note: This function has big overlap with the method
  // MatrixWorkspace::detectorTwoTheta(). The plan is to eventually remove the
  // latter, once GeometryInfo is in widespread use.
  const Kernel::V3D samplePos = m_factory.getSamplePos();
  const Kernel::V3D beamLine = samplePos - m_factory.getSourcePos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  return m_detector->getTwoTheta(samplePos, beamLine);
}

double GeometryInfo::getSignedTwoTheta() const {
  // Note: This function has big overlap with the method
  // MatrixWorkspace::detectorSignedTwoTheta(). The plan is to eventually remove
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
  return m_detector->getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
}

boost::shared_ptr<const Geometry::IDetector> GeometryInfo::getDetector() const {
  return m_detector;
}
}
}
