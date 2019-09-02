// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/EigenConversionHelpers.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include <numeric>

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

bool hasValidShape(const ObjCompAssembly &obj) {
  const auto *shape = obj.shape().get();
  return shape != nullptr && shape->hasValidShape();
}
} // namespace

/**
 * Constructor
 * @param instrument : Instrument being visited
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
      m_children(boost::make_shared<std::vector<std::vector<size_t>>>()),
      m_detectorRanges(
          boost::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentRanges(
          boost::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentIdToIndexMap(
          boost::make_shared<
              std::unordered_map<Mantid::Geometry::IComponent *, size_t>>()),
      m_detectorIdToIndexMap(makeDetIdToIndexMap(*m_orderedDetectorIds)),
      m_positions(boost::make_shared<std::vector<Eigen::Vector3d>>()),
      m_detectorPositions(boost::make_shared<std::vector<Eigen::Vector3d>>(
          m_orderedDetectorIds->size())),
      m_rotations(boost::make_shared<
                  std::vector<Eigen::Quaterniond,
                              Eigen::aligned_allocator<Eigen::Quaterniond>>>()),
      m_detectorRotations(
          boost::make_shared<
              std::vector<Eigen::Quaterniond,
                          Eigen::aligned_allocator<Eigen::Quaterniond>>>(
              m_orderedDetectorIds->size())),
      m_monitorIndices(boost::make_shared<std::vector<size_t>>()),
      m_instrument(std::move(instrument)), m_pmap(nullptr),
      m_nullShape(boost::make_shared<const CSGObject>()),
      m_shapes(
          boost::make_shared<std::vector<boost::shared_ptr<const IObject>>>(
              m_orderedDetectorIds->size(), m_nullShape)),
      m_scaleFactors(boost::make_shared<std::vector<Eigen::Vector3d>>(
          m_orderedDetectorIds->size(), Eigen::Vector3d{1, 1, 1})),
      m_componentType(
          boost::make_shared<std::vector<Beamline::ComponentType>>()),
      m_names(boost::make_shared<std::vector<std::string>>(
          m_orderedDetectorIds->size())) {
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
}

void InstrumentVisitor::walkInstrument() {
  if (m_pmap && m_pmap->empty()) {
    // Go through the base instrument for speed.
    m_instrument->baseInstrument()->registerContents(*this);
  } else
    m_instrument->registerContents(*this);
}

size_t InstrumentVisitor::commonRegistration(const IComponent &component) {
  const size_t componentIndex = m_componentIds->size();
  const ComponentID componentId = component.getComponentID();
  markAsSourceOrSample(componentId, componentIndex);
  // Record the ID -> index mapping
  (*m_componentIdToIndexMap)[componentId] = componentIndex;
  // For any non-detector we extend the m_componentIds from the back
  m_componentIds->emplace_back(componentId);
  m_positions->emplace_back(Kernel::toVector3d(component.getPos()));
  m_rotations->emplace_back(Kernel::toQuaterniond(component.getRotation()));
  m_shapes->emplace_back(m_nullShape);
  m_scaleFactors->emplace_back(Kernel::toVector3d(component.getScaleFactor()));
  m_names->emplace_back(component.getName());
  clearLegacyParameters(m_pmap, component);
  return componentIndex;
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
  const size_t componentIndex = commonRegistration(assembly);
  m_componentType->push_back(Beamline::ComponentType::Unstructured);
  m_assemblySortedComponentIndices->push_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->push_back(componentIndex);
  const size_t componentStop = m_assemblySortedComponentIndices->size();

  m_detectorRanges->emplace_back(std::make_pair(detectorStart, detectorStop));
  m_componentRanges->emplace_back(
      std::make_pair(componentStart, componentStop));

  // Now that we know what the index of the parent is we can apply it to the
  // children
  for (const auto &child : children) {
    (*m_parentComponentIndices)[child] = componentIndex;
  }
  m_children->emplace_back(std::move(children));
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
  const size_t componentIndex = commonRegistration(component);
  m_componentType->push_back(Beamline::ComponentType::Generic);

  const size_t componentStart = m_assemblySortedComponentIndices->size();
  m_componentRanges->emplace_back(
      std::make_pair(componentStart, componentStart + 1));
  m_assemblySortedComponentIndices->push_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->push_back(componentIndex);
  // Generic components are not assemblies and do not therefore have children.
  m_children->emplace_back(std::vector<size_t>());
  return componentIndex;
}

/**
 * @brief InstrumentVisitor::registerInfiniteComponent
 * @param component : IComponent being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerInfiniteComponent(
    const Mantid::Geometry::IComponent &component) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_detectorRanges->emplace_back(
      std::make_pair(0, 0)); // Represents an empty range
                             // Record the ID -> index mapping
  const size_t componentIndex = commonRegistration(component);
  m_componentType->push_back(Beamline::ComponentType::Infinite);

  const size_t componentStart = m_assemblySortedComponentIndices->size();
  m_componentRanges->emplace_back(
      std::make_pair(componentStart, componentStart + 1));
  m_assemblySortedComponentIndices->push_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->push_back(componentIndex);
  // Generic components are not assemblies and do not therefore have children.
  m_children->emplace_back(std::vector<size_t>());
  return componentIndex;
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
 * Register a rectangular bank
 * @param bank : Rectangular Detector
 * @return index assigned
 */
size_t InstrumentVisitor::registerRectangularBank(const ICompAssembly &bank) {
  auto index = registerComponentAssembly(bank);
  size_t rangesIndex = index - m_orderedDetectorIds->size();
  (*m_componentType)[rangesIndex] = Beamline::ComponentType::Rectangular;
  return index;
}

/**
 * Register a grid bank
 * @param bank : Grid Detector
 * @return index assigned
 */
size_t InstrumentVisitor::registerGridBank(const ICompAssembly &bank) {
  auto index = registerComponentAssembly(bank);
  size_t rangesIndex = index - m_orderedDetectorIds->size();
  (*m_componentType)[rangesIndex] = Beamline::ComponentType::Grid;
  return index;
}

/**
 * @brief InstrumentVisitor::registerInfiniteObjComponent
 * @param objComponent : IObjComponent being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerInfiniteObjComponent(
    const IObjComponent &objComponent) {
  auto index = registerInfiniteComponent(objComponent);
  (*m_shapes)[index] = objComponent.shape();
  return index;
}

/**
 * Register a structured bank
 * @param bank : Structured Detector
 * @return index assigned
 */
size_t InstrumentVisitor::registerStructuredBank(const ICompAssembly &bank) {
  auto index = registerComponentAssembly(bank);
  size_t rangesIndex = index - m_orderedDetectorIds->size();
  (*m_componentType)[rangesIndex] = Beamline::ComponentType::Structured;
  return index;
}

size_t
InstrumentVisitor::registerObjComponentAssembly(const ObjCompAssembly &obj) {
  auto index = registerComponentAssembly(obj);
  (*m_shapes)[index] = obj.shape();
  if (hasValidShape(obj)) {
    size_t rangesIndex = index - m_orderedDetectorIds->size();
    (*m_componentType)[rangesIndex] = Beamline::ComponentType::OutlineComposite;
  }
  return index;
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
 * @brief InstrumentVisitor::registerDetector
 * @param detector : IDetector being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerDetector(const IDetector &detector) {
  auto detectorIndex = m_detectorIdToIndexMap->at(detector.getID());

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
  (*m_detectorPositions)[detectorIndex] = Kernel::toVector3d(detector.getPos());
  (*m_detectorRotations)[detectorIndex] =
      Kernel::toQuaterniond(detector.getRotation());
  (*m_shapes)[detectorIndex] = detector.shape();
  (*m_scaleFactors)[detectorIndex] =
      Kernel::toVector3d(detector.getScaleFactor());
  if (m_instrument->isMonitorViaIndex(detectorIndex)) {
    m_monitorIndices->push_back(detectorIndex);
  }
  (*m_names)[detectorIndex] = detector.getName();
  clearLegacyParameters(m_pmap, detector);

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
size_t InstrumentVisitor::size() const { return m_componentIds->size(); }

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
  return std::make_unique<Mantid::Beamline::ComponentInfo>(
      m_assemblySortedDetectorIndices, m_detectorRanges,
      m_assemblySortedComponentIndices, m_componentRanges,
      m_parentComponentIndices, m_children, m_positions, m_rotations,
      m_scaleFactors, m_componentType, m_names, m_sourceIndex, m_sampleIndex);
}

std::unique_ptr<Beamline::DetectorInfo>
InstrumentVisitor::detectorInfo() const {
  return std::make_unique<Mantid::Beamline::DetectorInfo>(
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

  auto compInfoWrapper = std::make_unique<ComponentInfo>(
      std::move(compInfo), componentIds(), componentIdToIndexMap(), m_shapes);
  auto detInfoWrapper = std::make_unique<DetectorInfo>(
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
