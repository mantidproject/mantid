#ifndef MANTID_SINQ_POLDIDEADWIREDECORATOR_H_
#define MANTID_SINQ_POLDIDEADWIREDECORATOR_H_

#include "MantidKernel/System.h"

#include "MantidGeometry/Instrument.h"

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiDetectorDecorator.h"

namespace Mantid {
namespace Poldi {

/** PoldiDeadWireDecorator :
   *
   *This implementation of PoldiDetectorDecorator forwards all calls to the
   decorated detector,
   *except the ones regarding available elements. These are "cleaned" from dead
   wires which
   *have to be supplied in the form of std::set<int>

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
class MANTID_SINQ_DLL PoldiDeadWireDecorator : public PoldiDetectorDecorator {
public:
  PoldiDeadWireDecorator(std::set<int> deadWires,
                         boost::shared_ptr<PoldiAbstractDetector> detector =
                             boost::shared_ptr<PoldiAbstractDetector>());
  PoldiDeadWireDecorator(Geometry::Instrument_const_sptr poldiInstrument,
                         boost::shared_ptr<PoldiAbstractDetector> detector =
                             boost::shared_ptr<PoldiAbstractDetector>());

  virtual ~PoldiDeadWireDecorator() {}

  void setDeadWires(std::set<int> deadWires);
  std::set<int> deadWires();

  size_t elementCount();
  const std::vector<int> &availableElements();

protected:
  void detectorSetHook();
  std::vector<int> getGoodElements(std::vector<int> rawElements);

  static bool detectorIsNotMasked(Geometry::Instrument_const_sptr instrument,
                                  detid_t detectorId);
  bool isDeadElement(int index);

  std::set<int> m_deadWireSet;
  std::vector<int> m_goodElements;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIDEADWIREDECORATOR_H_ */
