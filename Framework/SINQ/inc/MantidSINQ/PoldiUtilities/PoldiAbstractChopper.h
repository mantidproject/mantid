// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"

#include "MantidGeometry/Instrument.h"

#include <utility>

namespace Mantid {
namespace Poldi {

/** PoldiAbstractChopper :
 *
  Abstract representation of the POLDI chopper

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 10/02/2014
*/

class MANTID_SINQ_DLL PoldiAbstractChopper {
public:
  virtual ~PoldiAbstractChopper() = default;

  virtual void loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument) = 0;

  virtual void setRotationSpeed(double rotationSpeed) = 0;

  virtual const std::vector<double> &slitPositions() = 0;
  virtual const std::vector<double> &slitTimes() = 0;

  virtual double rotationSpeed() = 0;
  virtual double cycleTime() = 0;
  virtual double zeroOffset() = 0;

  virtual double distanceFromSample() = 0;

protected:
  PoldiAbstractChopper() = default;
};

using PoldiAbstractChopper_sptr = std::shared_ptr<PoldiAbstractChopper>;
} // namespace Poldi
} // namespace Mantid
