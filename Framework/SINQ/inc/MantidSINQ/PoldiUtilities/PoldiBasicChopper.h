#ifndef MANTID_SINQ_POLDIBASICCHOPPER_H_
#define MANTID_SINQ_POLDIBASICCHOPPER_H_

#include "MantidKernel/System.h"

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"

namespace Mantid {
namespace Poldi {

/** PoldiBasicChopper :

    Implementation of PoldiAbstractChopper that models the currently installed
   device.
    Probably this will never change (tm), but just in case...

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 10/02/2014

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
class MANTID_SINQ_DLL PoldiBasicChopper : public PoldiAbstractChopper {
public:
  PoldiBasicChopper();
  ~PoldiBasicChopper() {}

  void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument);

  void setRotationSpeed(double rotationSpeed);

  const std::vector<double> &slitPositions();
  const std::vector<double> &slitTimes();

  double rotationSpeed();
  double cycleTime();
  double zeroOffset();

  double distanceFromSample();

protected:
  void initializeFixedParameters(std::vector<double> slitPositions,
                                 double distanceFromSample, double t0,
                                 double t0const);
  void initializeVariableParameters(double rotationSpeed);

  double slitPositionToTimeFraction(double slitPosition);

  // fixed parameters
  std::vector<double> m_slitPositions;
  double m_distanceFromSample;

  double m_rawt0;
  double m_rawt0const;

  // parameters that depend on rotation speed
  std::vector<double> m_slitTimes;

  double m_rotationSpeed;
  double m_cycleTime;
  double m_zeroOffset;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIBASICCHOPPER_H_ */
