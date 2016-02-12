#ifndef MANTID_ALGORITHMS_GEOMETRYINFO_H_
#define MANTID_ALGORITHMS_GEOMETRYINFO_H_

#include "MantidAlgorithms/BasicInstrumentInfo.h"

namespace Mantid {

namespace Geometry {
class Instrument;
class IDetector;
}

namespace API {
class ISpectrum;
}

namespace Algorithms {

class GeometryInfo {
public:
  GeometryInfo(const BasicInstrumentInfo &instrument_info,
               const API::ISpectrum &spectrum);

  bool isMonitor() const;
  bool isMasked() const;
  double getL1() const;
  double getL2() const;
  double getTwoTheta() const;
  double getSignedTwoTheta() const;
  boost::shared_ptr<const Geometry::IDetector> getDetector() const;

private:
  const BasicInstrumentInfo &m_instrument_info;
  boost::shared_ptr<const Geometry::IDetector> m_detector;
};
}
}

#endif /*MANTID_ALGORITHMS_GEOMETRYINFO_H_*/
