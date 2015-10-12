#ifndef POLDIHELIUMDETECTOR_H
#define POLDIHELIUMDETECTOR_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"

#include "MantidKernel/V2D.h"

namespace Mantid {
namespace Poldi {

/** PoldiHeliumDetector :
 *
  Implementation of PoldiAbstractDetector for the currently (2014) installed
 He3-based
  detector at the POLDI instrument.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 07/02/2014

    Copyright © 2014 PSI-MSS

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
using namespace Kernel;

class MANTID_SINQ_DLL PoldiHeliumDetector : public PoldiAbstractDetector {
public:
  PoldiHeliumDetector();
  ~PoldiHeliumDetector() {}

  void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument);

  double efficiency();

  double twoTheta(int elementIndex);
  double distanceFromSample(int elementIndex);

  size_t elementCount();
  size_t centralElement();

  const std::vector<int> &availableElements();

  std::pair<double, double> qLimits(double lambdaMin, double lambdaMax);

protected:
  double phi(int elementIndex);
  double phi(double twoTheta);

  void initializeFixedParameters(double radius, size_t elementCount,
                                 double elementWidth, double newEfficiency);
  void initializeCalibratedParameters(Kernel::V2D position,
                                      double centerTwoTheta);

  /* These detector parameters are fixed and specific to the geometry or result
   * from it directly */
  double m_radius;
  size_t m_elementCount;
  size_t m_centralElement;
  double m_elementWidth;
  double m_angularResolution;
  double m_totalOpeningAngle;
  std::vector<int> m_availableElements;
  double m_efficiency;

  /* Parameters that are calibrated or depend on calibrated parameters */
  V2D m_calibratedPosition;
  double m_vectorAngle;
  double m_distanceFromSample;

  double m_calibratedCenterTwoTheta;
  double m_phiCenter;
  double m_phiStart;
};
}
}

#endif // POLDIHELIUMDETECTOR_H
