#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"

#include <algorithm>
#include "boost/bind.hpp"

namespace Mantid {
namespace Poldi {

using namespace Geometry;

PoldiDeadWireDecorator::PoldiDeadWireDecorator(
    std::set<int> deadWires,
    boost::shared_ptr<Poldi::PoldiAbstractDetector> detector)
    : PoldiDetectorDecorator(detector), m_deadWireSet(deadWires),
      m_goodElements() {
  setDecoratedDetector(detector);
}

PoldiDeadWireDecorator::PoldiDeadWireDecorator(
    Instrument_const_sptr poldiInstrument,
    boost::shared_ptr<PoldiAbstractDetector> detector)
    : PoldiDetectorDecorator(detector), m_deadWireSet(), m_goodElements() {
  setDecoratedDetector(detector);

  std::vector<detid_t> allDetectorIds = poldiInstrument->getDetectorIDs();
  std::vector<detid_t> deadDetectorIds(allDetectorIds.size());

  std::vector<detid_t>::iterator endIterator = std::remove_copy_if(
      allDetectorIds.begin(), allDetectorIds.end(), deadDetectorIds.begin(),
      boost::bind<bool>(&PoldiDeadWireDecorator::detectorIsNotMasked,
                        poldiInstrument, _1));
  deadDetectorIds.resize(std::distance(deadDetectorIds.begin(), endIterator));

  setDeadWires(std::set<int>(deadDetectorIds.begin(), deadDetectorIds.end()));
}

void PoldiDeadWireDecorator::setDeadWires(std::set<int> deadWires) {
  m_deadWireSet = deadWires;

  detectorSetHook();
}

std::set<int> PoldiDeadWireDecorator::deadWires() { return m_deadWireSet; }

size_t PoldiDeadWireDecorator::elementCount() { return m_goodElements.size(); }

const std::vector<int> &PoldiDeadWireDecorator::availableElements() {
  return m_goodElements;
}

void PoldiDeadWireDecorator::detectorSetHook() {
  if (m_decoratedDetector) {
    m_goodElements = getGoodElements(m_decoratedDetector->availableElements());
  } else {
    throw(std::runtime_error("No decorated detector set!"));
  }
}

std::vector<int>
PoldiDeadWireDecorator::getGoodElements(std::vector<int> rawElements) {
  if (m_deadWireSet.size() > 0) {
    if (*m_deadWireSet.rbegin() > rawElements.back()) {
      throw std::runtime_error(
          std::string("Deadwires set contains illegal index."));
    }
    size_t newElementCount = rawElements.size() - m_deadWireSet.size();

    std::vector<int> goodElements(newElementCount);
    std::remove_copy_if(
        rawElements.begin(), rawElements.end(), goodElements.begin(),
        boost::bind<bool>(&PoldiDeadWireDecorator::isDeadElement, this, _1));

    return goodElements;
  }

  return rawElements;
}

bool
PoldiDeadWireDecorator::detectorIsNotMasked(Instrument_const_sptr instrument,
                                            detid_t detectorId) {
  return !instrument->isDetectorMasked(detectorId);
}

bool PoldiDeadWireDecorator::isDeadElement(int index) {
  return m_deadWireSet.count(index) != 0;
}

} // namespace Poldi
} // namespace Mantid
