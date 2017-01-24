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

  Usage:
  This is mainly intented for use in algorithms that need access to simple
  instrument parameters and work with spectra. For example:

  ~~~{.cpp}
  void exec() {
    // Some setup code
    GeometryInfoFactory factory(*inputWorkspace);
    // Loop over spectra
    for(size_t i=0; i<inputWorkspace->getNumberHistograms(); ++i) {
      auto geometry = factory.create(i);
      if (!geometry.isMasked()) {
        auto L1 = geometry.getL1();
        auto L2 = geometry.getL2();
        auto twoTheta = geometry.getTwoTheta();
        // Your code
      }
    }
  }
  ~~~

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
  /// Constructor, usually not used directy. Creation of GeometryInfo is done
  /// via GeometryInfoFactory.
  GeometryInfo(const GeometryInfoFactory &factory, const ISpectrum &spectrum);

  /// Returns true if the spectrum is a monitor.
  bool isMonitor() const;
  /// Returns true if the spectrum is masked.
  bool isMasked() const;
  /// Returns L1 (distance from source to sample).
  double getL1() const;
  /** Returns L2 (distance from sample to spectrum).
   *
   * For monitors this is defined such that L1+L2 = source-detector distance,
   * i.e., for a monitor in the beamline between source and sample L2 is
   * negative. */
  double getL2() const;
  /// Returns 2 theta (angle w.r.t. to beam direction).
  double getTwoTheta() const;
  /// Returns signed 2 theta (signed angle w.r.t. to beam direction).
  double getSignedTwoTheta() const;
  /// Returns the detector or detector group associated with the spectrum.
  boost::shared_ptr<const Geometry::IDetector> getDetector() const;

private:
  const GeometryInfoFactory &m_factory;
  boost::shared_ptr<const Geometry::IDetector> m_detector;
};
}
}

#endif /*MANTID_API_GEOMETRYINFO_H_*/
