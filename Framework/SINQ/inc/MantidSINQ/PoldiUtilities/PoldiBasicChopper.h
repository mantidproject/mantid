// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  */
class MANTID_SINQ_DLL PoldiBasicChopper : public PoldiAbstractChopper {
public:
  PoldiBasicChopper();

  void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument) override;

  void setRotationSpeed(double rotationSpeed) override;

  const std::vector<double> &slitPositions() override;
  const std::vector<double> &slitTimes() override;

  double rotationSpeed() override;
  double cycleTime() override;
  double zeroOffset() override;

  double distanceFromSample() override;

protected:
  void initializeFixedParameters(std::vector<double> slitPositions, double distanceFromSample, double t0,
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
