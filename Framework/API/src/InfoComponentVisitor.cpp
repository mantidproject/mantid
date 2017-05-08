#include "MantidAPI/InfoComponentVisitor.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"

#include <numeric>
#include <algorithm>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace API {

using namespace Mantid::Geometry;

InfoComponentVisitor::InfoComponentVisitor(const size_t nDetectors)
    : m_componentIds(
          boost::make_shared<std::vector<ComponentID>>(nDetectors, nullptr)),
      m_assemblySortedDetectorIndices(
          boost::make_shared<std::vector<size_t>>()),
      m_ranges(boost::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentIdToIndexMap(boost::make_shared<
          std::unordered_map<Mantid::Geometry::IComponent *, size_t>>()),
      m_detectorIdToIndexMap(
          boost::make_shared<std::unordered_map<detid_t, size_t>>()) {
  m_assemblySortedDetectorIndices->reserve(nDetectors);
  m_componentIdToIndexMap->reserve(nDetectors);
  m_detectorIdToIndexMap->reserve(nDetectors);
}

/**
 * @brief InfoComponentVisitor::registerComponentAssembly
 * @param assembly : ICompAssembly being visited
 */
void InfoComponentVisitor::registerComponentAssembly(
    const ICompAssembly &assembly) {

  std::vector<IComponent_const_sptr> assemblyChildren;
  assembly.getChildren(assemblyChildren, false /*is recursive*/);

  const size_t detectorStart = m_assemblySortedDetectorIndices->size();
  for (const auto &child : assemblyChildren) {
    // register everything under this assembly
    child->registerContents(*this);
  }
  const size_t detectorStop = m_assemblySortedDetectorIndices->size();

  m_ranges->emplace_back(std::make_pair(detectorStart, detectorStop));

  // Record the ID -> index mapping
  (*m_componentIdToIndexMap)[assembly.getComponentID()] =
      m_componentIds->size();
  // For any non-detector we extend the m_componetIds from the back
  m_componentIds->emplace_back(assembly.getComponentID());
}

/**
 * @brief InfoComponentVisitor::registerGenericComponent
 * @param component : IComponent being visited
 */
void InfoComponentVisitor::registerGenericComponent(
    const IComponent &component) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_ranges->emplace_back(std::make_pair(0, 0)); // Represents an empty range
  // Record the ID -> index mapping
  (*m_componentIdToIndexMap)[component.getComponentID()] =
      m_componentIds->size();
  m_componentIds->emplace_back(component.getComponentID());
}

/**
 * @brief InfoComponentVisitor::registerDetector
 * @param detector : IDetector being visited
 */
void InfoComponentVisitor::registerDetector(const IDetector &detector) {

  const size_t detectorIndex = m_assemblySortedDetectorIndices->size();
    /* Already allocated we just need to index into the inital front-detector
    * part of the collection.
    * 1. Guarantee on grouping detectors by type such that the first n
    * components
    * are detectors.
    * 2. Guarantee on ordering such that the
    * detectorIndex == componentIndex for all detectors.
    */
  // Record the ID -> component index mapping
  (*m_componentIdToIndexMap)[detector.getComponentID()] = detectorIndex;
  (*m_componentIds)[detectorIndex] = detector.getComponentID();
    // Record the det ID -> index mapping
  (*m_detectorIdToIndexMap)[detector.getID()] =
      detectorIndex; // register the detector index
  m_assemblySortedDetectorIndices->push_back(detectorIndex);
    // Increment counter for next registration
}

/**
 * @brief InfoComponentVisitor::componentDetectorRanges
 * @return index ranges into the detectorIndices vector. Gives the
 * intervals of detectors indices for non-detector components such as banks
 */
boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
InfoComponentVisitor::componentDetectorRanges() const {
  return m_ranges;
}

/**
 * @brief InfoComponentVisitor::detectorIndices
 * @return detector indices in the order in which they have been visited
 * thus grouped by assembly to form a contiguous range for levels of assemblies.
 */
boost::shared_ptr<const std::vector<size_t>>
InfoComponentVisitor::assemblySortedDetectorIndices() const {
  return m_assemblySortedDetectorIndices;
}

/**
 * @brief InfoComponentVisitor::componentIds
 * @return  component ids in the order in which they have been visited.
 * Note that the number of component ids will be >= the number of detector
 * indices
 * since all detectors are components but not all components are detectors
 */
boost::shared_ptr<const std::vector<Mantid::Geometry::ComponentID>>
InfoComponentVisitor::componentIds() const {
  return m_componentIds;
}

/**
 * @brief InfoComponentVisitor::size
 * @return The total size of the components visited.
 * This will be the same as the number of IDs.
 */
size_t InfoComponentVisitor::size() const { return m_componentIds->size(); }

boost::shared_ptr<
    const std::unordered_map<Mantid::Geometry::IComponent *, size_t>>
InfoComponentVisitor::componentIdToIndexMap() const {
  return m_componentIdToIndexMap;
}

boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
InfoComponentVisitor::detectorIdToIndexMap() const {
  return m_detectorIdToIndexMap;
}
} // namespace API
} // namespace Mantid
