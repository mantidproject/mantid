#include "MantidAPI/InfoComponentVisitor.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidBeamline/ComponentInfo.h"

#include <numeric>
#include <algorithm>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace API {

using namespace Mantid::Geometry;

namespace {

void clearPositionAndRotationsParameters(ParameterMap &pmap,
                                         const IComponent &comp) {
  pmap.clearParametersByName(ParameterMap::pos(), &comp);
  pmap.clearParametersByName(ParameterMap::posx(), &comp);
  pmap.clearParametersByName(ParameterMap::posy(), &comp);
  pmap.clearParametersByName(ParameterMap::posz(), &comp);
  pmap.clearParametersByName(ParameterMap::rot(), &comp);
  pmap.clearParametersByName(ParameterMap::rotx(), &comp);
  pmap.clearParametersByName(ParameterMap::roty(), &comp);
  pmap.clearParametersByName(ParameterMap::rotz(), &comp);
}
}

InfoComponentVisitor::InfoComponentVisitor(
    const size_t nDetectors,
    std::function<size_t(const Mantid::detid_t)> mapperFunc, ParameterMap &pmap)
    : m_componentIds(nDetectors, nullptr),
      m_detectorIdToIndexMapperFunction(mapperFunc),
      m_positions(boost::make_shared<std::vector<Eigen::Vector3d>>()),
      m_rotations(boost::make_shared<std::vector<Eigen::Quaterniond>>()),
      m_pmap(pmap) {
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

  m_positions->emplace_back(Kernel::toVector3d(assembly.getPos()));
  m_rotations->emplace_back(Kernel::toQuaterniond(assembly.getRotation()));
  clearPositionAndRotationsParameters(m_pmap, assembly);
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
  m_positions->emplace_back(Kernel::toVector3d(component.getPos()));
  m_rotations->emplace_back(Kernel::toQuaterniond(component.getRotation()));
  clearPositionAndRotationsParameters(m_pmap, component);
}

/**
 * @brief InfoComponentVisitor::registerDetector
 * @param detector : IDetector being visited
 */
void InfoComponentVisitor::registerDetector(const IDetector &detector) {

  const auto detectorIndex =
      m_detectorIdToIndexMapperFunction(detector.getID());

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
  /* Note that positions and rotations for detectors are currently
  NOT stored! These go into DetectorInfo at present*/
}

const std::vector<size_t> &
InfoComponentVisitor::componentSortedDetectorIndices() const {
  return m_assemblySortedDetectorIndices;
}

const std::vector<std::pair<size_t, size_t>> &
InfoComponentVisitor::componentDetectorRanges() const {
  return m_ranges;
}

boost::shared_ptr<std::vector<Eigen::Vector3d>>
InfoComponentVisitor::positions() const {
  return m_positions;
}

boost::shared_ptr<std::vector<Eigen::Quaterniond>>
InfoComponentVisitor::rotations() const {
  return m_rotations;
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
size_t InfoComponentVisitor::size() const { return m_componentIds.size(); }

} // namespace API
} // namespace Mantid
