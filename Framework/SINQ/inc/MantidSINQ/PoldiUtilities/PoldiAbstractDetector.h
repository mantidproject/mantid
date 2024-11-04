// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"

#include "MantidGeometry/Instrument.h"

#include "MantidKernel/V2D.h"

#include <utility>

namespace Mantid {
namespace Poldi {

/** PoldiAbstractDetector :
 *
  Abstract representation of detector, used for POLDI related calculations.
 Calculation
  methods are the repsonsibility of concrete implementations.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 07/02/2014
*/

class MANTID_SINQ_DLL PoldiAbstractDetector {
public:
  virtual ~PoldiAbstractDetector() = default;

  virtual void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument) = 0;

  virtual double efficiency() = 0;

  virtual double twoTheta(int elementIndex) = 0;
  virtual double distanceFromSample(int elementIndex) = 0;

  virtual size_t elementCount() = 0;
  virtual size_t centralElement() = 0;

  virtual const std::vector<int> &availableElements() = 0;

  virtual std::pair<double, double> qLimits(double lambdaMin, double lambdaMax) = 0;

protected:
  PoldiAbstractDetector() = default;
};

using PoldiAbstractDetector_sptr = std::shared_ptr<PoldiAbstractDetector>;
} // namespace Poldi
} // namespace Mantid
