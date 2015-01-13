#ifndef POLDIABSTRACTDETECTOR_H
#define POLDIABSTRACTDETECTOR_H

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

class MANTID_SINQ_DLL PoldiAbstractDetector {
public:
  virtual ~PoldiAbstractDetector() {}

  virtual void
  loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument) = 0;

  virtual double efficiency() = 0;

  virtual double twoTheta(int elementIndex) = 0;
  virtual double distanceFromSample(int elementIndex) = 0;

  virtual size_t elementCount() = 0;
  virtual size_t centralElement() = 0;

  virtual const std::vector<int> &availableElements() = 0;

  virtual std::pair<double, double> qLimits(double lambdaMin,
                                            double lambdaMax) = 0;

protected:
  PoldiAbstractDetector() {}
};

typedef boost::shared_ptr<PoldiAbstractDetector> PoldiAbstractDetector_sptr;
}
}
#endif // POLDIABSTRACTDETECTOR_H
