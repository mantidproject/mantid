// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"

namespace Mantid {
namespace Poldi {

/** PoldiDetectorDecorator :

    Decorator interface for POLDI detectors. The base implementation just
   forwards all calls
    to the decorated detector object.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 12/02/2014
  */
class MANTID_SINQ_DLL PoldiDetectorDecorator : public PoldiAbstractDetector {
public:
  PoldiDetectorDecorator(
      std::shared_ptr<PoldiAbstractDetector> decoratedDetector = std::shared_ptr<PoldiAbstractDetector>());

  void setDecoratedDetector(std::shared_ptr<PoldiAbstractDetector> detector);
  std::shared_ptr<PoldiAbstractDetector> decoratedDetector();

  void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument) override;

  double efficiency() override;

  double twoTheta(int elementIndex) override;
  double distanceFromSample(int elementIndex) override;

  size_t elementCount() override;
  size_t centralElement() override;

  const std::vector<int> &availableElements() override;

  std::pair<double, double> qLimits(double lambdaMin, double lambdaMax) override;

protected:
  virtual void detectorSetHook();

  std::shared_ptr<PoldiAbstractDetector> m_decoratedDetector;
};

} // namespace Poldi
} // namespace Mantid
