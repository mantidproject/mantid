#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/make_unique.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"

#include <numeric>
#include <algorithm>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

namespace {
boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
makeDetIdToIndexMap(const std::vector<detid_t> &detIds) {

  const size_t nDetIds = detIds.size();
  auto detIdToIndex = boost::make_shared<std::unordered_map<detid_t, size_t>>();
  detIdToIndex->reserve(nDetIds);
  for (size_t i = 0; i < nDetIds; ++i) {
    (*detIdToIndex)[detIds[i]] = i;
  }
  return std::move(detIdToIndex);
}

void clearLegacyParameters(ParameterMap *pmap, const IComponent &comp) {
  if (!pmap)
    return;
  pmap->clearParametersByName(ParameterMap::pos(), &comp);
  pmap->clearParametersByName(ParameterMap::posx(), &comp);
  pmap->clearParametersByName(ParameterMap::posy(), &comp);
  pmap->clearParametersByName(ParameterMap::posz(), &comp);
  pmap->clearParametersByName(ParameterMap::rot(), &comp);
  pmap->clearParametersByName(ParameterMap::rotx(), &comp);
  pmap->clearParametersByName(ParameterMap::roty(), &comp);
  pmap->clearParametersByName(ParameterMap::rotz(), &comp);
  pmap->clearParametersByName(ParameterMap::scale(), &comp);
}
}

/**
 * @brief InstrumentVisitor::registerComponentAssembly
 * @param instrument : Instrument being visited
 * @return Component index of this component
 */
InstrumentVisitor::InstrumentVisitor(
    boost::shared_ptr<const Instrument> instrument)
    : m_orderedDetectorIds(boost::make_shared<std::vector<detid_t>>(
          instrument->getDetectorIDs(false /*Do not skip monitors*/))),
      m_componentIds(boost::make_shared<std::vector<ComponentID>>(
          m_orderedDetectorIds->size(), nullptr)),
      m_assemblySortedDetectorIndices(
          boost::make_shared<std::vector<size_t>>()),
      m_assemblySortedComponentIndices(
          boost::make_shared<std::vector<size_t>>()),
      m_parentComponentIndices(boost::make_shared<std::vector<size_t>>(
          m_orderedDetectorIds->size(), 0)),
      m_detectorRanges(
          boost::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentRanges(
          boost::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentIdToIndexMap(boost::make_shared<
          std::unordered_map<Mantid::Geometry::IComponent *, size_t>>()),
      m_detectorIdToIndexMap(makeDetIdToIndexMap(*m_orderedDetectorIds)),
      m_positions(boost::make_shared<std::vector<Eigen::Vector3d>>()),
      m_detectorPositions(boost::make_shared<std::vector<Eigen::Vector3d>>(
          m_orderedDetectorIds->size())),
      m_rotations(boost::make_shared<std::vector<Eigen::Quaterniond>>()),
      m_detectorRotations(boost::make_shared<std::vector<Eigen::Quaterniond>>(
          m_orderedDetectorIds->size())),
      m_monitorIndices(boost::make_shared<std::vector<size_t>>()),
      m_instrument(std::move(instrument)), m_pmap(nullptr),
      m_nullShape(boost::make_shared<const Object>()),
      m_shapes(boost::make_shared<std::vector<boost::shared_ptr<const Object>>>(
          m_orderedDetectorIds->size(), m_nullShape)),
      m_scaleFactors(boost::make_shared<std::vector<Eigen::Vector3d>>(
          m_orderedDetectorIds->size(), Eigen::Vector3d{1, 1, 1})) {

  if (m_instrument->isParametrized()) {
    m_pmap = m_instrument->getParameterMap().get();
  }

  m_sourceId = nullptr;
  m_sampleId = nullptr;

  // To prevent warning generation. Do not even try to get the source or sample
  // id if the instrument is empty.
  if (!m_instrument->isEmptyInstrument()) {

    m_sourceId = m_instrument->getSource()
                     ? m_instrument->getSource()->getComponentID()
                     : nullptr;
    m_sampleId = m_instrument->getSample()
                     ? m_instrument->getSample()->getComponentID()
                     : nullptr;
  }

  const auto nDetectors = m_orderedDetectorIds->size();
  m_assemblySortedDetectorIndices->reserve(nDetectors); // Exact
  m_componentIdToIndexMap->reserve(nDetectors);         // Approximation
  m_shapes->reserve(nDetectors);                        // Approximation
}

void InstrumentVisitor::walkInstrument() {
  if (m_pmap && m_pmap->empty()) {
    // Go through the base instrument for speed.
    m_instrument->baseInstrument()->registerContents(*this);
  } else
    m_instrument->registerContents(*this);
}

size_t
InstrumentVisitor::registerComponentAssembly(const ICompAssembly &assembly) {

  std::vector<IComponent_const_sptr> assemblyChildren;
  assembly.getChildren(assemblyChildren, false /*is recursive*/);

  const size_t detectorStart = m_assemblySortedDetectorIndices->size();
  const size_t componentStart = m_assemblySortedComponentIndices->size();
  std::vector<size_t> children(assemblyChildren.size());
  for (size_t i = 0; i < assemblyChildren.size(); ++i) {
    // register everything under this assembly
    children[i] = assemblyChildren[i]->registerContents(*this);
  }
  const size_t detectorStop = m_assemblySortedDetectorIndices->size();
  const size_t componentIndex = m_componentIds->size();
  m_assemblySortedComponentIndices->push_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->push_back(componentIndex);
  const size_t componentStop = m_assemblySortedComponentIndices->size();

  m_detectorRanges->emplace_back(std::make_pair(detectorStart, detectorStop));
  m_componentRanges->emplace_back(
      std::make_pair(componentStart, componentStop));

  // Record the ID -> index mapping
  (*m_componentIdToIndexMap)[assembly.getComponentID()] = componentIndex;
  // For any non-detector we extend the m_componentIds from the back
  m_componentIds->emplace_back(assembly.getComponentID());
  m_positions->emplace_back(Kernel::toVector3d(assembly.getPos()));
  m_rotations->emplace_back(Kernel::toQuaterniond(assembly.getRotation()));
  // Now that we know what the index of the parent is we can apply it to the
  // children
  for (const auto &child : children) {
    (*m_parentComponentIndices)[child] = componentIndex;
  }
  markAsSourceOrSample(assembly.getComponentID(), componentIndex);
  m_shapes->emplace_back(m_nullShape);
  m_scaleFactors->emplace_back(Kernel::toVector3d(assembly.getScaleFactor()));
  clearLegacyParameters(m_pmap, assembly);
  return componentIndex;
}

/**
 * @brief InstrumentVisitor::registerGenericComponent
 * @param component : IComponent being visited
 * @return Component index of this component
 */
size_t
InstrumentVisitor::registerGenericComponent(const IComponent &component) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_detectorRanges->emplace_back(
      std::make_pair(0, 0)); // Represents an empty range
  // Record the ID -> index mapping
  const size_t componentIndex = m_componentIds->size();
  (*m_componentIdToIndexMap)[component.getComponentID()] = componentIndex;
  m_componentIds->emplace_back(component.getComponentID());
  m_positions->emplace_back(Kernel::toVector3d(component.getPos()));
  m_rotations->emplace_back(Kernel::toQuaterniond(component.getRotation()));
  const size_t componentStart = m_assemblySortedComponentIndices->size();
  m_componentRanges->emplace_back(
      std::make_pair(componentStart, componentStart + 1));
  m_assemblySortedComponentIndices->push_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->push_back(componentIndex);
  markAsSourceOrSample(component.getComponentID(), componentIndex);
  m_shapes->emplace_back(m_nullShape);
  m_scaleFactors->emplace_back(Kernel::toVector3d(component.getScaleFactor()));
  clearLegacyParameters(m_pmap, component);
  return componentIndex;
}

void InstrumentVisitor::markAsSourceOrSample(ComponentID componentId,
                                             const size_t componentIndex) {
  if (componentId == m_sampleId) {
    m_sampleIndex = componentIndex;
  } else if (componentId == m_sourceId) {
    m_sourceIndex = componentIndex;
  }
}

/**
 * @brief InstrumentVisitor::registerGenericObjComponent
 * @param objComponent : IObjComponent being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerGenericObjComponent(
    const Mantid::Geometry::IObjComponent &objComponent) {
  auto index = registerGenericComponent(objComponent);
  (*m_shapes)[index] = objComponent.shape();
  return index;
}

/**
 * @brief InstrumentVisitor::registerDetector
 * @param detector : IDetector being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerDetector(const IDetector &detector) {

  size_t detectorIndex = 0;
  try {
    detectorIndex = m_detectorIdToIndexMap->at(detector.getID());
  } catch (std::out_of_range &) {
    /*
     Do not register a detector with an invalid id. if we can't determine
     the index, we cannot register it in the right place!
    */
    ++m_droppedDetectors;
    return detectorIndex;
  }
  if (m_componentIds->at(detectorIndex) == nullptr) {

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
    m_assemblySortedDetectorIndices->push_back(detectorIndex);
    (*m_detectorPositions)[detectorIndex] =
        Kernel::toVector3d(detector.getPos());
    (*m_detectorRotations)[detectorIndex] =
        Kernel::toQuaterniond(detector.getRotation());
    (*m_shapes)[detectorIndex] = detector.shape();
    (*m_scaleFactors)[detectorIndex] =
        Kernel::toVector3d(detector.getScaleFactor());
    if (m_instrument->isMonitorViaIndex(detectorIndex)) {
      m_monitorIndices->push_back(detectorIndex);
    }
    clearLegacyParameters(m_pmap, detector);
  }
  /* Note that positions and rotations for detectors are currently
  NOT stored! These go into DetectorInfo at present. push_back works for other
  Component types because Detectors are always come first in the resultant
  component list
  forming a contiguous block.
  */
  markAsSourceOrSample(detector.getComponentID(),
                       detectorIndex); // TODO. Optimisation. Cannot have a
                                       // detector that is either source or
                                       // sample. So delete this.
  return detectorIndex;
}

/**
 * @brief InstrumentVisitor::componentIds
 * @return  component ids in the order in which they have been visited.
 * Note that the number of component ids will be >= the number of detector
 * indices
 * since all detectors are components but not all components are detectors
 */
boost::shared_ptr<const std::vector<Mantid::Geometry::ComponentID>>
InstrumentVisitor::componentIds() const {
  return m_componentIds;
}

/**
 * @brief InstrumentVisitor::size
 * @return The total size of the components visited.
 * This will be the same as the number of IDs.
 */
size_t InstrumentVisitor::size() const {
  return m_componentIds->size() - m_droppedDetectors;
}

bool InstrumentVisitor::isEmpty() const { return size() == 0; }

boost::shared_ptr<
    const std::unordered_map<Mantid::Geometry::IComponent *, size_t>>
InstrumentVisitor::componentIdToIndexMap() const {
  return m_componentIdToIndexMap;
}

boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
InstrumentVisitor::detectorIdToIndexMap() const {
  return m_detectorIdToIndexMap;
}

std::unique_ptr<Beamline::ComponentInfo>
InstrumentVisitor::componentInfo() const {
  return Kernel::make_unique<Mantid::Beamline::ComponentInfo>(
      m_assemblySortedDetectorIndices, m_detectorRanges,
      m_assemblySortedComponentIndices, m_componentRanges,
      m_parentComponentIndices, m_positions, m_rotations, m_scaleFactors,
      m_sourceIndex, m_sampleIndex);
}

std::unique_ptr<Beamline::DetectorInfo>
InstrumentVisitor::detectorInfo() const {
  return Kernel::make_unique<Mantid::Beamline::DetectorInfo>(
      *m_detectorPositions, *m_detectorRotations, *m_monitorIndices);
}

boost::shared_ptr<std::vector<detid_t>> InstrumentVisitor::detectorIds() const {
  return m_orderedDetectorIds;
}

std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
InstrumentVisitor::makeWrappers() const {
  auto compInfo = componentInfo();
  auto detInfo = detectorInfo();
  // Cross link Component and Detector info objects
  compInfo->setDetectorInfo(detInfo.get());
  detInfo->setComponentInfo(compInfo.get());

  auto compInfoWrapper = Kernel::make_unique<ComponentInfo>(
      std::move(compInfo), componentIds(), componentIdToIndexMap(), m_shapes);
  auto detInfoWrapper = Kernel::make_unique<DetectorInfo>(
      std::move(detInfo), m_instrument, detectorIds(), detectorIdToIndexMap());

  return {std::move(compInfoWrapper), std::move(detInfoWrapper)};
}

std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
InstrumentVisitor::makeWrappers(const Instrument &instrument,
                                ParameterMap *pmap) {
  // Visitee instrument is base instrument if no ParameterMap
  const auto visiteeInstrument =
      pmap ? ParComponentFactory::createInstrument(
                 boost::shared_ptr<const Instrument>(&instrument, NoDeleting()),
                 boost::shared_ptr<ParameterMap>(pmap, NoDeleting()))
           : boost::shared_ptr<const Instrument>(&instrument, NoDeleting());
  InstrumentVisitor visitor(visiteeInstrument);
  visitor.walkInstrument();
  return visitor.makeWrappers();
}

} // namespace Geometry
} // namespace Mantid
