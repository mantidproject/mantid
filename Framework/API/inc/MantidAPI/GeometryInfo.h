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

/** GeometryInfo is a small wrapper around the Instrument/Geometry and provides
  easy access to commonly used parameters, such as L1, L2, and 2-theta.

  @author Simon Heybrock, ESS

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_API_DLL GeometryInfo {
public:
  GeometryInfo(const GeometryInfoFactory &instrument_info,
               const ISpectrum &spectrum);

  /// Returns true is the spectrum is a monitor.
  bool isMonitor() const;
  /// Returns true is the spectrum is masked.
  bool isMasked() const;
  /// Returns L1 (distance from source to sample).
  double getL1() const;
  /// Returns L2 (distance from sample to spectrum).
  double getL2() const;
  /// Returns 2 theta (angle w.r.t. to beam direction).
  double getTwoTheta() const;
  /// Returns signed 2 theta (signed angle w.r.t. to beam direction).
  double getSignedTwoTheta() const;
  /// Returns the detector or detector group associated with the spectrum.
  boost::shared_ptr<const Geometry::IDetector> getDetector() const;

private:
  const GeometryInfoFactory &m_instrument_info;
  boost::shared_ptr<const Geometry::IDetector> m_detector;
};
}
}

#endif /*MANTID_API_GEOMETRYINFO_H_*/
