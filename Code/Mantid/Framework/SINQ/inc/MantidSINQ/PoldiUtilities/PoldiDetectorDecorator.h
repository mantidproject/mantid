#ifndef MANTID_SINQ_POLDIDETECTORDECORATOR_H_
#define MANTID_SINQ_POLDIDETECTORDECORATOR_H_

#include "MantidKernel/System.h"
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
class MANTID_SINQ_DLL PoldiDetectorDecorator : public PoldiAbstractDetector {
public:
  PoldiDetectorDecorator(
      boost::shared_ptr<PoldiAbstractDetector> decoratedDetector =
          boost::shared_ptr<PoldiAbstractDetector>());

  virtual ~PoldiDetectorDecorator() {}

  void setDecoratedDetector(boost::shared_ptr<PoldiAbstractDetector> detector);
  boost::shared_ptr<PoldiAbstractDetector> decoratedDetector();

  virtual void
  loadConfiguration(Geometry::Instrument_const_sptr poldiInstrument);

  virtual double efficiency();

  virtual double twoTheta(int elementIndex);
  virtual double distanceFromSample(int elementIndex);

  virtual size_t elementCount();
  virtual size_t centralElement();

  virtual const std::vector<int> &availableElements();

  virtual std::pair<double, double> qLimits(double lambdaMin, double lambdaMax);

protected:
  virtual void detectorSetHook();

  boost::shared_ptr<PoldiAbstractDetector> m_decoratedDetector;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIDETECTORDECORATOR_H_ */
