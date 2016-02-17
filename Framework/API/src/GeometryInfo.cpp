#include "MantidAPI/GeometryInfo.h"

#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid {
namespace API {

GeometryInfo::GeometryInfo(const GeometryInfoFactory &instrument_info,
                           const ISpectrum &spectrum)
    : m_instrument_info(instrument_info) {

  // Note: This constructor body has big overlap with the method
  // MatrixWorkspace::getDetector(). The plan is to eventually remove the
  // latter, once GeometryInfo is in widespread use.
  const std::set<detid_t> &dets = spectrum.getDetectorIDs();

  const size_t ndets = dets.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    m_detector = m_instrument_info.getInstrument().getDetector(*dets.begin());
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           "");
  } else {
    // Else need to construct a DetectorGroup and use that
    std::vector<Geometry::IDetector_const_sptr> dets_ptr =
        m_instrument_info.getInstrument().getDetectors(dets);
    m_detector = Geometry::IDetector_const_sptr(
        new Geometry::DetectorGroup(dets_ptr, false));
  }
}

bool GeometryInfo::isMonitor() const { return m_detector->isMonitor(); }

bool GeometryInfo::isMasked() const { return m_detector->isMasked(); }

double GeometryInfo::getL1() const { return m_instrument_info.getL1(); }

double GeometryInfo::getL2() const {
  if (!isMonitor()) {
    auto &sample = m_instrument_info.getSample();
    return m_detector->getDistance(sample);
  } else {
    // If this is a monitor then make l1+l2 = source-detector distance
    auto &source = m_instrument_info.getSource();
    return m_detector->getDistance(source) - getL1();
  }
}

double GeometryInfo::getTwoTheta() const {
  const Kernel::V3D samplePos = m_instrument_info.getSamplePos();
  const Kernel::V3D beamLine = samplePos - m_instrument_info.getSourcePos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  return m_detector->getTwoTheta(samplePos, beamLine);
}

double GeometryInfo::getSignedTwoTheta() const {
  const Kernel::V3D samplePos = m_instrument_info.getSamplePos();
  const Kernel::V3D beamLine = samplePos - m_instrument_info.getSourcePos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const Kernel::V3D &instrumentUpAxis =
      m_instrument_info.getInstrument().getReferenceFrame()->vecPointingUp();
  return m_detector->getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
}

boost::shared_ptr<const Geometry::IDetector> GeometryInfo::getDetector() const {
  return m_detector;
}
}
}
