// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidGeometry/Instrument/PixelAssembly.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/EigenConversionHelpers.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <numeric>

namespace Mantid::Geometry {

namespace {
std::shared_ptr<const std::unordered_map<detid_t, size_t>> makeDetIdToIndexMap(const std::vector<detid_t> &detIds) {

  const size_t nDetIds = detIds.size();
  auto detIdToIndex = std::make_shared<std::unordered_map<detid_t, size_t>>();
  detIdToIndex->reserve(nDetIds);
  for (size_t i = 0; i < nDetIds; ++i) {
    (*detIdToIndex)[detIds[i]] = i;
  }
  return detIdToIndex;
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
InstrumentVisitor::InstrumentVisitor(std::shared_ptr<const Instrument> instrument)
    : m_orderedDetectorIds(
          std::make_shared<std::vector<detid_t>>(instrument->getDetectorIDs(false /*Do not skip monitors*/))),
      m_componentIds(std::make_shared<std::vector<ComponentID>>(m_orderedDetectorIds->size(), nullptr)),
      m_assemblySortedDetectorIndices(std::make_shared<std::vector<size_t>>()),
      m_assemblySortedComponentIndices(std::make_shared<std::vector<size_t>>()),
      m_parentComponentIndices(std::make_shared<std::vector<size_t>>(m_orderedDetectorIds->size(), 0)),
      m_children(std::make_shared<std::vector<std::vector<size_t>>>()),
      m_detectorRanges(std::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentRanges(std::make_shared<std::vector<std::pair<size_t, size_t>>>()),
      m_componentIdToIndexMap(std::make_shared<std::unordered_map<Mantid::Geometry::IComponent const *, size_t>>()),
      m_detectorIdToIndexMap(makeDetIdToIndexMap(*m_orderedDetectorIds)),
      m_positions(std::make_shared<std::vector<Eigen::Vector3d>>()),
      m_detectorPositions(std::make_shared<std::vector<Eigen::Vector3d>>(m_orderedDetectorIds->size())),
      m_rotations(std::make_shared<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>()),
      m_detectorRotations(
          std::make_shared<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>(
              m_orderedDetectorIds->size())),
      m_monitorIndices(std::make_shared<std::vector<size_t>>()), m_instrument(std::move(instrument)), m_pmap(nullptr),
      m_nullShape(std::make_shared<const CSGObject>()),
      m_shapes(
          std::make_shared<std::vector<std::shared_ptr<const IObject>>>(m_orderedDetectorIds->size(), m_nullShape)),
      m_scaleFactors(
          std::make_shared<std::vector<Eigen::Vector3d>>(m_orderedDetectorIds->size(), Eigen::Vector3d{1, 1, 1})),
      m_componentType(std::make_shared<std::vector<Beamline::ComponentType>>()),
      m_names(std::make_shared<std::vector<std::string>>(m_orderedDetectorIds->size())) {
  if (m_instrument->isParametrized()) {
    m_pmap = m_instrument->getParameterMap().get();
  }

  m_sourceId = nullptr;
  m_sampleId = nullptr;

  // To prevent warning generation. Do not even try to get the source or sample
  // id if the instrument is empty.
  if (!m_instrument->isEmptyInstrument()) {

    m_sourceId = m_instrument->getSource() ? m_instrument->getSource()->getComponentID() : nullptr;
    m_sampleId = m_instrument->getSample() ? m_instrument->getSample()->getComponentID() : nullptr;
  }

  const auto nDetectors = m_orderedDetectorIds->size();
  m_assemblySortedDetectorIndices->reserve(nDetectors); // Exact
  m_componentIdToIndexMap->reserve(nDetectors);         // Approximation
}

void InstrumentVisitor::walkInstrument() {
  if (m_pmap && m_pmap->empty()) {
    // Go through the base instrument for speed.
    m_instrument->baseInstrument()->registerContents(*this);
  } else {
    m_instrument->registerContents(*this);
  }
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

size_t InstrumentVisitor::registerComponentAssembly(const ICompAssembly &assembly) {

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
  m_componentType->emplace_back(Beamline::ComponentType::Unstructured);
  m_assemblySortedComponentIndices->emplace_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->emplace_back(componentIndex);
  const size_t componentStop = m_assemblySortedComponentIndices->size();

  m_detectorRanges->emplace_back(detectorStart, detectorStop);
  m_componentRanges->emplace_back(componentStart, componentStop);

  // Now that we know what the index of the parent is we can apply it to the children
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
size_t InstrumentVisitor::registerGenericComponent(const IComponent &component) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_detectorRanges->emplace_back(std::make_pair(0, 0)); // Represents an empty range
  // Record the ID -> index mapping
  const size_t componentIndex = commonRegistration(component);
  m_componentType->emplace_back(Beamline::ComponentType::Generic);

  const size_t componentStart = m_assemblySortedComponentIndices->size();
  m_componentRanges->emplace_back(std::make_pair(componentStart, componentStart + 1));
  m_assemblySortedComponentIndices->emplace_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->emplace_back(componentIndex);
  // Generic components are not assemblies and do not therefore have children.
  m_children->emplace_back();
  return componentIndex;
}

/**
 * @brief InstrumentVisitor::registerInfiniteComponent
 * @param component : IComponent being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerInfiniteComponent(const Mantid::Geometry::IComponent &component) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_detectorRanges->emplace_back(0, 0); // Represents an empty range
                                        // Record the ID -> index mapping
  const size_t componentIndex = commonRegistration(component);
  m_componentType->emplace_back(Beamline::ComponentType::Infinite);

  const size_t componentStart = m_assemblySortedComponentIndices->size();
  m_componentRanges->emplace_back(componentStart, componentStart + 1);
  m_assemblySortedComponentIndices->emplace_back(componentIndex);
  // Unless this is the root component this parent is not correct and will be
  // updated later in the register call of the parent.
  m_parentComponentIndices->emplace_back(componentIndex);
  // Generic components are not assemblies and do not therefore have children.
  m_children->emplace_back();
  return componentIndex;
}

/**
 * @brief InstrumentVisitor::registerGenericObjComponent
 * @param objComponent : IObjComponent being visited
 * @return Component index of this component
 */
size_t InstrumentVisitor::registerGenericObjComponent(const Mantid::Geometry::IObjComponent &objComponent) {
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
 * Register a virtual bank (PixelAssembly) — no per-pixel child objects exist.
 *
 * Iterates over the grid parameters to fill detector positions and rotations
 * directly, without creating Detector objects for each pixel.
 *
 * NOTE: m_componentIds[detectorIndex] remains nullptr for virtual-bank pixels
 * (there are no heap-allocated IComponent objects for them).  Code that uses
 * the legacy ComponentID pointer for virtual pixels must handle null.
 *
 * @param bank : PixelAssembly being registered
 * @return component index assigned to the bank
 */
size_t InstrumentVisitor::registerVirtualBank(const PixelAssembly &bank) {
  const size_t detectorStart = m_assemblySortedDetectorIndices->size();
  const size_t componentStart = m_assemblySortedComponentIndices->size();

  // All pixels share the same shape and rotation.
  auto const pxShape = bank.pixelShape() ? bank.pixelShape() : m_nullShape;
  // Bank absolute rotation and position (world frame).
  Eigen::Quaterniond const eigenRot = Kernel::toQuaterniond(bank.getRotation());
  Eigen::Vector3d const bankPos = Kernel::toVector3d(bank.getPos());

  // Collect detector indices without materialising per-pixel positions.
  // Positions are stored as a VirtualBankSegment and computed on demand in
  // Beamline::DetectorInfo::position(size_t) — no flat-array entries are
  // written for virtual pixels.
  std::vector<size_t> detectorChildren;
  detectorChildren.reserve(bank.npixels());

  size_t firstDetectorIndex = std::numeric_limits<size_t>::max();
  size_t lastDetectorIndex = 0;

  for (size_t iz = 0; iz < bank.zpixels(); ++iz) {
    for (size_t ix = 0; ix < bank.xpixels(); ++ix) {
      for (size_t iy = 0; iy < bank.ypixels(); ++iy) {
        detid_t const id = bank.getDetectorIDAtXYZ(static_cast<int>(ix), static_cast<int>(iy), static_cast<int>(iz));
        size_t const detectorIndex = m_detectorIdToIndexMap->at(id);
        detectorChildren.push_back(detectorIndex);
        m_assemblySortedDetectorIndices->emplace_back(detectorIndex);
        firstDetectorIndex = std::min(firstDetectorIndex, detectorIndex);
        lastDetectorIndex = std::max(lastDetectorIndex, detectorIndex);

        // Shape, name, scale factor, and position are NOT written per-pixel:
        // handled lazily by VirtualBankSegment lookups.
      }
    }
  }

  // Register the bank as a virtual segment so DetectorInfo and ComponentInfo
  // can compute pixel positions/rotations/parents on demand.
  // bankCompIdx is only known after commonRegistration() returns.
  const size_t detectorStop = m_assemblySortedDetectorIndices->size();
  const size_t componentIndex = commonRegistration(bank);
  m_componentType->emplace_back(Beamline::ComponentType::Grid);
  m_assemblySortedComponentIndices->emplace_back(componentIndex);
  m_parentComponentIndices->emplace_back(componentIndex);
  const size_t componentStop = m_assemblySortedComponentIndices->size();

  m_detectorRanges->emplace_back(detectorStart, detectorStop);
  m_componentRanges->emplace_back(componentStart, componentStop);
  m_children->emplace_back(std::move(detectorChildren));

  if (bank.npixels() > 0) {
    Beamline::VirtualBankSegment seg;
    seg.firstIndex = firstDetectorIndex;
    seg.lastIndex = lastDetectorIndex;
    seg.bankCompIdx = componentIndex; // now known
    seg.bankPos = bankPos;
    seg.bankRot = eigenRot;
    seg.nx = static_cast<int>(bank.xpixels());
    seg.ny = static_cast<int>(bank.ypixels());
    seg.xstart = bank.xstart();
    seg.xstep = bank.xstep();
    seg.ystart = bank.ystart();
    seg.ystep = bank.ystep();
    m_virtualBankSegments.push_back(std::move(seg));
    // Record the pixel shape once per bank (not per pixel).
    m_virtualPixelShapes.push_back(pxShape);
  }

  return componentIndex;
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
size_t InstrumentVisitor::registerInfiniteObjComponent(const IObjComponent &objComponent) {
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

size_t InstrumentVisitor::registerObjComponentAssembly(const ObjCompAssembly &obj) {
  auto index = registerComponentAssembly(obj);
  (*m_shapes)[index] = obj.shape();
  if (hasValidShape(obj)) {
    size_t rangesIndex = index - m_orderedDetectorIds->size();
    (*m_componentType)[rangesIndex] = Beamline::ComponentType::OutlineComposite;
  }
  return index;
}

void InstrumentVisitor::markAsSourceOrSample(ComponentID componentId, const size_t componentIndex) {
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
  m_assemblySortedDetectorIndices->emplace_back(detectorIndex);
  (*m_detectorPositions)[detectorIndex] = Kernel::toVector3d(detector.getPos());
  (*m_detectorRotations)[detectorIndex] = Kernel::toQuaterniond(detector.getRotation());
  (*m_shapes)[detectorIndex] = detector.shape();
  (*m_scaleFactors)[detectorIndex] = Kernel::toVector3d(detector.getScaleFactor());
  // Use isMonitor(id) rather than isMonitorViaIndex(index): after Phase 3c the
  // detector cache contains only non-virtual detectors, so its size no longer
  // equals the total detector count, and index-based access would be incorrect.
  if (m_instrument->isMonitor(detector.getID())) {
    m_monitorIndices->emplace_back(detectorIndex);
  }
  (*m_names)[detectorIndex] = detector.getName();
  clearLegacyParameters(m_pmap, detector);

  /* Note that positions and rotations for detectors are currently
  NOT stored! These go into DetectorInfo at present. emplace_back works for
  other Component types because Detectors are always come first in the resultant
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
std::shared_ptr<const std::vector<Mantid::Geometry::ComponentID>> InstrumentVisitor::componentIds() const {
  return m_componentIds;
}

/**
 * @brief InstrumentVisitor::size
 * @return The total size of the components visited.
 * This will be the same as the number of IDs.
 */
size_t InstrumentVisitor::size() const { return m_componentIds->size(); }

bool InstrumentVisitor::isEmpty() const { return size() == 0; }

std::shared_ptr<const std::unordered_map<Mantid::Geometry::IComponent const *, size_t>>
InstrumentVisitor::componentIdToIndexMap() const {
  return m_componentIdToIndexMap;
}

std::shared_ptr<const std::unordered_map<detid_t, size_t>> InstrumentVisitor::detectorIdToIndexMap() const {
  return m_detectorIdToIndexMap;
}

std::unique_ptr<Beamline::ComponentInfo> InstrumentVisitor::componentInfo() const {
  if (m_virtualBankSegments.empty()) {
    // No virtual banks: pass full-size arrays unchanged.
    return std::make_unique<Mantid::Beamline::ComponentInfo>(
        m_assemblySortedDetectorIndices, m_detectorRanges, m_assemblySortedComponentIndices, m_componentRanges,
        m_parentComponentIndices, m_children, m_positions, m_rotations, m_scaleFactors, m_componentType, m_names,
        m_sourceIndex, m_sampleIndex);
  }

  // Sort segments by firstIndex (they may have been registered in any order).
  auto sortedSegs = m_virtualBankSegments;
  std::sort(sortedSegs.begin(), sortedSegs.end(),
            [](const Beamline::VirtualBankSegment &a, const Beamline::VirtualBankSegment &b) {
              return a.firstIndex < b.firstIndex;
            });

  // Build an is-virtual mask for quick per-index lookup.
  const size_t nDetectors = m_orderedDetectorIds->size();
  std::vector<bool> isVirtual(nDetectors, false);
  for (const auto &seg : sortedSegs) {
    for (size_t i = seg.firstIndex; i <= seg.lastIndex; ++i)
      isVirtual[i] = true;
  }

  // ------------------------------------------------------------------
  // Compact m_names, m_scaleFactors, m_parentComponentIndices:
  // keep only non-virtual detector entries + all non-detector entries.
  // ------------------------------------------------------------------
  auto compactNames = std::make_shared<std::vector<std::string>>();
  auto compactSF = std::make_shared<std::vector<Eigen::Vector3d>>();
  auto compactParents = std::make_shared<std::vector<size_t>>();

  const size_t nNonVirtual = std::count(isVirtual.begin(), isVirtual.end(), false);
  const size_t nNonDetectors = m_positions->size(); // non-detector components only
  const size_t compactSize = nNonVirtual + nNonDetectors;

  compactNames->reserve(compactSize);
  compactSF->reserve(compactSize);
  compactParents->reserve(compactSize);

  // Non-virtual detector slice
  for (size_t i = 0; i < nDetectors; ++i) {
    if (!isVirtual[i]) {
      compactNames->push_back((*m_names)[i]);
      compactSF->push_back((*m_scaleFactors)[i]);
      compactParents->push_back((*m_parentComponentIndices)[i]);
    }
  }
  // Non-detector component slice (appended after detectors in the originals)
  for (size_t i = nDetectors; i < m_parentComponentIndices->size(); ++i) {
    compactNames->push_back((*m_names)[i]);
    compactSF->push_back((*m_scaleFactors)[i]);
    compactParents->push_back((*m_parentComponentIndices)[i]);
  }

  return std::make_unique<Mantid::Beamline::ComponentInfo>(
      m_assemblySortedDetectorIndices, m_detectorRanges, m_assemblySortedComponentIndices, m_componentRanges,
      std::shared_ptr<const std::vector<size_t>>(compactParents), m_children, m_positions, m_rotations,
      std::shared_ptr<std::vector<Eigen::Vector3d>>(compactSF), m_componentType,
      std::shared_ptr<const std::vector<std::string>>(compactNames), m_sourceIndex, m_sampleIndex,
      std::move(sortedSegs));
}

std::unique_ptr<Beamline::DetectorInfo> InstrumentVisitor::detectorInfo() const {
  if (m_virtualBankSegments.empty()) {
    // No virtual banks: use the original constructor (no compaction needed).
    return std::make_unique<Mantid::Beamline::DetectorInfo>(*m_detectorPositions, *m_detectorRotations,
                                                            *m_monitorIndices);
  }

  // Sort segments by firstIndex so that findVirtualSegment() can break early.
  auto sortedSegs = m_virtualBankSegments;
  std::sort(sortedSegs.begin(), sortedSegs.end(),
            [](const Beamline::VirtualBankSegment &a, const Beamline::VirtualBankSegment &b) {
              return a.firstIndex < b.firstIndex;
            });

  // Build a compact boolean mask: true for virtual pixel indices.
  const size_t nTotal = m_detectorPositions->size();
  std::vector<bool> isVirtual(nTotal, false);
  size_t nVirtual = 0;
  for (const auto &seg : sortedSegs) {
    for (size_t i = seg.firstIndex; i <= seg.lastIndex; ++i)
      isVirtual[i] = true;
    nVirtual += seg.lastIndex - seg.firstIndex + 1;
  }

  // Compact arrays: copy only real (non-virtual) detector positions/rotations.
  using QuatVec = std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>;
  std::vector<Eigen::Vector3d> compactPos;
  QuatVec compactRot;
  compactPos.reserve(nTotal - nVirtual);
  compactRot.reserve(nTotal - nVirtual);
  for (size_t i = 0; i < nTotal; ++i) {
    if (!isVirtual[i]) {
      compactPos.push_back((*m_detectorPositions)[i]);
      compactRot.push_back((*m_detectorRotations)[i]);
    }
  }

  return std::make_unique<Mantid::Beamline::DetectorInfo>(std::move(compactPos), std::move(compactRot),
                                                          *m_monitorIndices, std::move(sortedSegs));
}

std::shared_ptr<std::vector<detid_t>> InstrumentVisitor::detectorIds() const { return m_orderedDetectorIds; }

std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>> InstrumentVisitor::makeWrappers() const {
  auto compInfo = componentInfo();
  auto detInfo = detectorInfo();
  // Cross link Component and Detector info objects
  compInfo->setDetectorInfo(detInfo.get());

  // For virtual-bank instruments: build compact m_componentIds and m_shapes
  // (omit the nullptr/placeholder entries for virtual pixel slots).
  std::shared_ptr<const std::vector<Geometry::IComponent *>> compactIds;
  std::shared_ptr<std::vector<std::shared_ptr<const Geometry::IObject>>> compactShapes;

  if (!m_virtualBankSegments.empty()) {
    const size_t nDetectors = m_orderedDetectorIds->size();

    // Build is-virtual mask (re-use logic from componentInfo()/detectorInfo()).
    std::vector<bool> isVirtual(nDetectors, false);
    for (const auto &seg : m_virtualBankSegments)
      for (size_t i = seg.firstIndex; i <= seg.lastIndex; ++i)
        isVirtual[i] = true;

    const size_t nNonVirtual = static_cast<size_t>(std::count(isVirtual.begin(), isVirtual.end(), false));
    const size_t nNonDetectors = m_positions->size();
    const size_t compactSize = nNonVirtual + nNonDetectors;

    auto ids = std::make_shared<std::vector<Geometry::IComponent *>>();
    ids->reserve(compactSize);
    auto shapes = std::make_shared<std::vector<std::shared_ptr<const Geometry::IObject>>>();
    shapes->reserve(compactSize);

    // Non-virtual detector entries first, then all non-detector components.
    for (size_t i = 0; i < nDetectors; ++i) {
      if (!isVirtual[i]) {
        ids->push_back((*m_componentIds)[i]);
        shapes->push_back((*m_shapes)[i]);
      }
    }
    for (size_t i = nDetectors; i < m_componentIds->size(); ++i) {
      ids->push_back((*m_componentIds)[i]);
      shapes->push_back((*m_shapes)[i]);
    }

    compactIds = std::move(ids);
    compactShapes = std::move(shapes);
  } else {
    compactIds = componentIds();
    compactShapes = m_shapes;
  }

  auto compInfoWrapper = std::make_unique<ComponentInfo>(std::move(compInfo), compactIds, componentIdToIndexMap(),
                                                         compactShapes, m_virtualPixelShapes);
  auto detInfoWrapper =
      std::make_unique<DetectorInfo>(std::move(detInfo), m_instrument, detectorIds(), detectorIdToIndexMap());

  return {std::move(compInfoWrapper), std::move(detInfoWrapper)};
}

std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
InstrumentVisitor::makeWrappers(const Instrument &instrument, ParameterMap *pmap) {
  // Visitee instrument is base instrument if no ParameterMap
  const auto visiteeInstrument =
      pmap ? ParComponentFactory::createInstrument(std::shared_ptr<const Instrument>(&instrument, NoDeleting()),
                                                   std::shared_ptr<ParameterMap>(pmap, NoDeleting()))
           : std::shared_ptr<const Instrument>(&instrument, NoDeleting());
  InstrumentVisitor visitor(visiteeInstrument);
  visitor.walkInstrument();
  return visitor.makeWrappers();
}
} // namespace Mantid::Geometry
