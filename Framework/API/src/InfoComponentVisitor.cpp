#include "MantidAPI/InfoComponentVisitor.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"

#include <numeric>
#include <algorithm>

namespace Mantid {
namespace API {

using namespace Mantid::Geometry;

InfoComponentVisitor::InfoComponentVisitor(
    const size_t nDetectors,
    std::function<size_t(const Mantid::detid_t)> mapperFunc)
    : m_componentIds(nDetectors, nullptr),
      m_detectorIdToIndexMapperFunction(mapperFunc) {
  m_assemblySortedDetectorIndices.reserve(nDetectors);
}

/**
 * @brief InfoComponentVisitor::registerComponentAssembly
 * @param assembly : ICompAssembly being visited
 */
void InfoComponentVisitor::registerComponentAssembly(
    const ICompAssembly &assembly) {

  std::vector<IComponent_const_sptr> assemblyChildren;
  assembly.getChildren(assemblyChildren, false /*is recursive*/);

  const size_t detectorStart = m_assemblySortedDetectorIndices.size();
  for (const auto &child : assemblyChildren) {
    // register everything under this assembly
    child->registerContents(*this);
  }
  const size_t detectorStop = m_assemblySortedDetectorIndices.size();

  m_ranges.emplace_back(std::make_pair(detectorStart, detectorStop));

  // For any non-detector we extend the m_componetIds from the back
  m_componentIds.emplace_back(assembly.getComponentID());
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
  m_ranges.emplace_back(std::make_pair(0, 0)); // Represents an empty range
  m_componentIds.emplace_back(component.getComponentID());
}

/**
 * @brief InfoComponentVisitor::registerDetector
 * @param detector : IDetector being visited
 */
void InfoComponentVisitor::registerDetector(const IDetector &detector) {

  size_t detectorIndex = 0;
  try {
    detectorIndex = m_detectorIdToIndexMapperFunction(detector.getID());
  } catch (std::out_of_range &) {
    /*
     Do not register a detector with an invalid id. if we can't determine
     the index, we cannot register it in the right place!
    */
    ++m_droppedDetectors;
    return;
  }

  /* Unfortunately Mantid supports having detectors attached to an
   * instrument that have an an invalid or duplicate detector id.
   * We do not register detectors with the same id twice.
   */
  if (m_componentIds[detectorIndex] == nullptr) {

    /* Already allocated we just need to index into the inital front-detector
    * part of the collection.
    * 1. Guarantee on grouping detectors by type such that the first n
    * components
    * are detectors.
    * 2. Guarantee on ordering such that the
    * detectorIndex == componentIndex for all detectors.
    */
    m_componentIds[detectorIndex] = detector.getComponentID();

    // register the detector index
    m_assemblySortedDetectorIndices.push_back(detectorIndex);
  }
}

/**
 * @brief InfoComponentVisitor::componentDetectorRanges
 * @return index ranges into the detectorIndices vector. Gives the
 * intervals of detectors indices for non-detector components such as banks
 */
const std::vector<std::pair<size_t, size_t>> &
InfoComponentVisitor::componentDetectorRanges() const {
  return m_ranges;
}

/**
 * @brief InfoComponentVisitor::detectorIndices
 * @return detector indices in the order in which they have been visited
 * thus grouped by assembly to form a contiguous range for levels of assemblies.
 */
const std::vector<size_t> &
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
const std::vector<Mantid::Geometry::ComponentID> &
InfoComponentVisitor::componentIds() const {
  return m_componentIds;
}

/**
 * @brief InfoComponentVisitor::size
 * @return The total size of the components visited.
 * This will be the same as the number of IDs.
 */
size_t InfoComponentVisitor::size() const {
  return m_componentIds.size() - m_droppedDetectors;
}

} // namespace API
} // namespace Mantid
