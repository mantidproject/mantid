// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDIDEADWIREDECORATOR_H_
#define MANTID_SINQ_POLDIDEADWIREDECORATOR_H_

#include "MantidKernel/System.h"

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiDetectorDecorator.h"

namespace Mantid {
namespace Geometry {
class DetectorInfo;
}
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
  */
class MANTID_SINQ_DLL PoldiDeadWireDecorator : public PoldiDetectorDecorator {
public:
  PoldiDeadWireDecorator(std::set<int> deadWires,
                         boost::shared_ptr<PoldiAbstractDetector> detector =
                             boost::shared_ptr<PoldiAbstractDetector>());
  PoldiDeadWireDecorator(const Geometry::DetectorInfo &poldiDetectorInfo,
                         boost::shared_ptr<PoldiAbstractDetector> detector =
                             boost::shared_ptr<PoldiAbstractDetector>());

  void setDeadWires(std::set<int> deadWires);
  std::set<int> deadWires();

  size_t elementCount() override;
  const std::vector<int> &availableElements() override;

protected:
  void detectorSetHook() override;
  std::vector<int> getGoodElements(std::vector<int> rawElements);

  bool isDeadElement(int index);

  std::set<int> m_deadWireSet;
  std::vector<int> m_goodElements;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIDEADWIREDECORATOR_H_ */
