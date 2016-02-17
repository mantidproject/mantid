#ifndef MANTID_API_GEOMETRYINFO_H_
#define MANTID_API_GEOMETRYINFO_H_

#include "MantidAPI/GeometryInfoFactory.h"

namespace Mantid {

namespace Geometry {
class Instrument;
class IDetector;
}

namespace API {
class ISpectrum;

class MANTID_API_DLL GeometryInfo {
public:
  GeometryInfo(const GeometryInfoFactory &instrument_info,
               const ISpectrum &spectrum);

  bool isMonitor() const;
  bool isMasked() const;
  double getL1() const;
  double getL2() const;
  double getTwoTheta() const;
  double getSignedTwoTheta() const;
  boost::shared_ptr<const Geometry::IDetector> getDetector() const;

private:
  const GeometryInfoFactory &m_instrument_info;
  boost::shared_ptr<const Geometry::IDetector> m_detector;
};
}
}

#endif /*MANTID_API_GEOMETRYINFO_H_*/
