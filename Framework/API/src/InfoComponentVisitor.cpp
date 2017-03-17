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
    : m_componentIds(detectorInfo.size()), m_detectorInfo(detectorInfo) {}

void InfoComponentVisitor::registerComponentAssembly(
    const ICompAssembly &bank, std::vector<size_t> &parentDetectorIndexes) {

  // Local cache of immediate child detector indexes
  std::vector<size_t> localDetectorIndexes;
  std::vector<IComponent_const_sptr> assemblyChildren;
  bank.getChildren(assemblyChildren, false /*is recursive*/);

  for (const auto &child : assemblyChildren) {
    // register everything under this assembly
    child->registerContents(*this, localDetectorIndexes);
  }

  if (localDetectorIndexes.size() > 0) {
    /* We need confidence that we will be able to generate a range that
     * represents a contiguous block. With more confidence we could remove this.
    */
    if (!std::is_sorted(localDetectorIndexes.begin(),
                        localDetectorIndexes.end())) {
      for (auto index : localDetectorIndexes) {
        std::cout << index << " ";
      }
      throw std::runtime_error(
          "We expect an ascending list of detector indices for each assembly");
    }
    if (localDetectorIndexes.back() !=
        localDetectorIndexes.front() + (localDetectorIndexes.size() - 1)) {
      throw std::runtime_error("Detector indices should be increasing +1 for "
                               "all local detectors in the assembly");
    }
    const auto startIndex = localDetectorIndexes.front();
    const auto endIndex = startIndex + localDetectorIndexes.size();
    m_ranges.emplace_back(std::make_pair(startIndex, endIndex));
  } else {
    m_ranges.emplace_back(std::make_pair(0, 0));
  }
  // For any non-detector we extend the m_componetIds from the back
  m_componentIds.emplace_back(bank.getComponentID());

  /*
   * The following allows us to refer to ALL nested
   * detectors from any component in the tree. Otherwise these could
   * be hidden by sub-tree assemblies.
   */
  parentDetectorIndexes.reserve(parentDetectorIndexes.size() +
                                localDetectorIndexes.size());
  parentDetectorIndexes.insert(parentDetectorIndexes.end(),
                               localDetectorIndexes.begin(),
                               localDetectorIndexes.end());
}

void InfoComponentVisitor::registerGenericComponent(const IComponent &component,
                                                    std::vector<size_t> &) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_ranges.emplace_back(std::make_pair(0, 0)); // Represents an empty range
  m_componentIds.emplace_back(component.getComponentID());
}
void InfoComponentVisitor::registerDetector(
    const IDetector &detector, std::vector<size_t> &parentDetectorIndexes) {

  // detectorIndex == componentIndex
  auto componentIndex = m_detectorInfo.indexOf(detector.getID());

  /* Already allocated we just need to index into the inital front-detector
   * part of the collection
  */
  m_componentIds[componentIndex] = detector.getComponentID();

  parentDetectorIndexes.push_back(componentIndex);
}

std::vector<std::pair<size_t, size_t>>
InfoComponentVisitor::componentDetectorRanges() const {
  return m_ranges;
}

std::vector<Mantid::Geometry::ComponentID>
InfoComponentVisitor::componentIds() const {
  return m_componentIds;
}

size_t InfoComponentVisitor::size() const { return m_componentIds.size(); }

} // namespace API
} // namespace Mantid
