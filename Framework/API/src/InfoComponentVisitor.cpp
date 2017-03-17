#include "MantidAPI/InfoComponentVisitor.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"

#include <numeric>
#include <algorithm>
#include <iostream>

namespace Mantid {
namespace API {

using namespace Mantid::Geometry;

InfoComponentVisitor::InfoComponentVisitor(
    const Mantid::API::DetectorInfo &detectorInfo)
    : m_componentIds(detectorInfo.size()), m_detectorInfo(detectorInfo),
      m_detectorCounter(0) {
  m_detectorIndices.reserve(detectorInfo.size());
}

void InfoComponentVisitor::registerComponentAssembly(
    const ICompAssembly &bank) {

  std::vector<IComponent_const_sptr> assemblyChildren;
  bank.getChildren(assemblyChildren, false /*is recursive*/);

  const size_t detectorStart = m_detectorIndices.size();
  for (const auto &child : assemblyChildren) {
    // register everything under this assembly
    child->registerContents(*this);
  }
  const size_t detectorStop = m_detectorIndices.size();

  m_ranges.emplace_back(std::make_pair(detectorStart, detectorStop));

  // For any non-detector we extend the m_componetIds from the back
  m_componentIds.emplace_back(bank.getComponentID());
}

void InfoComponentVisitor::registerGenericComponent(
    const IComponent &component) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_ranges.emplace_back(std::make_pair(0, 0)); // Represents an empty range
  m_componentIds.emplace_back(component.getComponentID());
}
void InfoComponentVisitor::registerDetector(const IDetector &detector) {

  const auto detectorIndex = m_detectorInfo.indexOf(detector.getID());

  /* Already allocated we just need to index into the inital front-detector
   * part of the collection
  */
  m_componentIds[m_detectorCounter++] = detector.getComponentID();
  // register the detector index
  m_detectorIndices.push_back(detectorIndex);
}

std::vector<std::pair<size_t, size_t>>
InfoComponentVisitor::componentDetectorRanges() const {
  return m_ranges;
}

std::vector<size_t> InfoComponentVisitor::detectorIndices() const {
  return m_detectorIndices;
}

std::vector<Mantid::Geometry::ComponentID>
InfoComponentVisitor::componentIds() const {
  return m_componentIds;
}

size_t InfoComponentVisitor::size() const { return m_componentIds.size(); }

} // namespace API
} // namespace Mantid
