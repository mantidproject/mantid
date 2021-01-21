// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/

class MANTID_SINQ_DLL PoldiHeliumDetector : public PoldiAbstractDetector {
public:
  PoldiHeliumDetector();

  void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument) override;

  double efficiency() override;

  double twoTheta(int elementIndex) override;
  double distanceFromSample(int elementIndex) override;

  size_t elementCount() override;
  size_t centralElement() override;

  const std::vector<int> &availableElements() override;

  std::pair<double, double> qLimits(double lambdaMin, double lambdaMax) override;

protected:
  double phi(int elementIndex);
  double phi(double twoTheta);

  void initializeFixedParameters(double radius, size_t elementCount, double elementWidth, double newEfficiency);
  void initializeCalibratedParameters(Kernel::V2D position, double centerTwoTheta);

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
  Kernel::V2D m_calibratedPosition;
  double m_vectorAngle;
  double m_distanceFromSample;

  double m_calibratedCenterTwoTheta;
  double m_phiCenter;
  double m_phiStart;
};
} // namespace Poldi
} // namespace Mantid
