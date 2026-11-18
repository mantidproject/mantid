// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/BeamlineBuilder.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <ranges>
#include <stdexcept>
#include <unordered_map>

namespace Mantid::Geometry {

using Kernel::Quat;
using Kernel::V2D;
using Kernel::V3D;

namespace {
Kernel::Logger g_log("BeamlineBuilder");
}

/// Lays out the nodes as InstrumentVisitor does, filling the arrays the Infos are made from.
class BeamlineBuilder::Layout {
public:
  using Rotations = std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>;

  explicit Layout(BeamlineBuilder const &builder) : m_builder(builder), m_nodeIndices(builder.m_nodes.size()) {
    auto const &nodes = builder.m_nodes;
    // Detectors take the leading indices, in detector ID order
    std::vector<Node> detectors;
    std::ranges::copy_if(std::views::iota(Node{0}, nodes.size()), std::back_inserter(detectors),
                         [&nodes](Node const node) { return nodes[node].isDetector; });
    std::sort(detectors.begin(), detectors.end(),
              [&nodes](Node const a, Node const b) { return nodes[a].id < nodes[b].id; });
    auto const duplicate =
        std::adjacent_find(detectors.cbegin(), detectors.cend(),
                           [&nodes](Node const a, Node const b) { return nodes[a].id == nodes[b].id; });
    if (duplicate != detectors.cend()) {
      throw Kernel::Exception::InstrumentDefinitionError(
          "BeamlineBuilder: detector ID " + std::to_string(nodes[*duplicate].id) + " is used more than once");
    }
    auto const nDetectors = detectors.size();
    m_detectorIds = std::make_shared<std::vector<detid_t>>(nDetectors);
    auto detectorIdToIndex = std::make_shared<std::unordered_map<detid_t, size_t>>();
    detectorIdToIndex->reserve(nDetectors);
    for (size_t index = 0; index < nDetectors; ++index) {
      auto const id = nodes[detectors[index]].id;
      (*m_detectorIds)[index] = id;
      (*detectorIdToIndex)[id] = index;
      m_nodeIndices[detectors[index]] = index;
    }
    m_detectorIdToIndex = std::move(detectorIdToIndex);

    m_parentIndices = std::make_shared<std::vector<size_t>>(nDetectors, 0);
    m_detectorPositions.resize(nDetectors);
    m_detectorRotations.resize(nDetectors);
    m_shapes = std::make_shared<std::vector<std::shared_ptr<const IObject>>>(nDetectors, m_nullShape);
    m_scaleFactors = std::make_shared<std::vector<Eigen::Vector3d>>(nDetectors, Eigen::Vector3d{1, 1, 1});
    m_names = std::make_shared<std::vector<std::string>>(nDetectors);
    m_assemblySortedDetectorIndices->reserve(nDetectors);

    placeNodes();
    visit(0);
  }

  std::unique_ptr<Beamline::ComponentInfo> componentInfo() const {
    return std::make_unique<Beamline::ComponentInfo>(
        m_assemblySortedDetectorIndices, m_detectorRanges, m_assemblySortedComponentIndices, m_componentRanges,
        m_parentIndices, m_children, m_positions, m_rotations, m_scaleFactors, m_componentTypes, m_names,
        m_sideBySideViewPositions, m_pixelGridComponents, index(m_builder.m_source), index(m_builder.m_sample),
        m_structuredGridComponents);
  }

  /// Takes the detector positions and rotations, so call it once.
  std::unique_ptr<Beamline::DetectorInfo> detectorInfo() {
    return std::make_unique<Beamline::DetectorInfo>(std::move(m_detectorPositions), std::move(m_detectorRotations),
                                                    m_monitorIndices);
  }

  std::shared_ptr<std::vector<std::shared_ptr<const IObject>>> shapes() const { return m_shapes; }
  std::shared_ptr<const std::vector<detid_t>> detectorIds() const { return m_detectorIds; }
  std::shared_ptr<const std::unordered_map<detid_t, size_t>> detectorIdToIndex() const { return m_detectorIdToIndex; }
  size_t index(Node const node) const { return m_nodeIndices[node]; }

private:
  int64_t index(std::optional<Node> const &node) const {
    return node ? static_cast<int64_t>(m_nodeIndices[*node]) : -1;
  }

  /// Absolute positions, rotations and side-by-side view positions, composed as the legacy tree composes them.
  /// A parent is always added before its children, so one forward pass suffices.
  void placeNodes() {
    auto const &nodes = m_builder.m_nodes;
    m_absolutePositions.resize(nodes.size());
    m_absoluteRotations.resize(nodes.size());
    m_absoluteSideBySide.resize(nodes.size());
    for (Node node = 0; node < nodes.size(); ++node) {
      auto const &record = nodes[node];
      if (record.parent == noParent) {
        m_absolutePositions[node] = record.position;
        m_absoluteRotations[node] = record.rotation;
        m_absoluteSideBySide[node] = record.sideBySideViewPosition;
        continue;
      }
      V3D position(record.position);
      m_absoluteRotations[record.parent].rotate(position);
      m_absolutePositions[node] = position + m_absolutePositions[record.parent];
      m_absoluteRotations[node] = m_absoluteRotations[record.parent] * record.rotation;
      auto const &parentSideBySide = m_absoluteSideBySide[record.parent];
      if (record.sideBySideViewPosition || parentSideBySide) {
        m_absoluteSideBySide[node] =
            record.sideBySideViewPosition.value_or(V2D(0., 0.)) + parentSideBySide.value_or(V2D(0., 0.));
      }
    }
  }

  /// Register a node and everything under it, returning its component index.
  size_t visit(Node const node) {
    auto const &record = m_builder.m_nodes[node];
    if (record.isDetector) {
      return visitDetector(node);
    }
    if (!record.isAssembly) {
      m_detectorRanges->emplace_back(0, 0);
      auto const componentIndex = commonRegistration(node);
      auto const componentStart = m_assemblySortedComponentIndices->size();
      m_componentRanges->emplace_back(componentStart, componentStart + 1);
      m_assemblySortedComponentIndices->emplace_back(componentIndex);
      m_parentIndices->emplace_back(componentIndex);
      m_children->emplace_back();
      return componentIndex;
    }
    auto const detectorStart = m_assemblySortedDetectorIndices->size();
    auto const componentStart = m_assemblySortedComponentIndices->size();
    auto const &childNodes = m_builder.m_children[node];
    std::vector<size_t> children(childNodes.size());
    for (size_t i = 0; i < childNodes.size(); ++i) {
      children[i] = visit(childNodes[i]);
    }
    auto const detectorStop = m_assemblySortedDetectorIndices->size();
    auto const componentIndex = commonRegistration(node);
    m_assemblySortedComponentIndices->emplace_back(componentIndex);
    // The root is its own parent; any other parent is set when its parent is registered
    m_parentIndices->emplace_back(componentIndex);
    auto const componentStop = m_assemblySortedComponentIndices->size();
    m_detectorRanges->emplace_back(detectorStart, detectorStop);
    m_componentRanges->emplace_back(componentStart, componentStop);
    for (auto const child : children) {
      (*m_parentIndices)[child] = componentIndex;
    }
    m_children->emplace_back(std::move(children));
    if (record.pixelGrid) {
      (*m_pixelGridComponents)[componentIndex] = *record.pixelGrid;
    }
    if (record.structuredGrid) {
      (*m_structuredGridComponents)[componentIndex] = *record.structuredGrid;
    }
    return componentIndex;
  }

  size_t visitDetector(Node const node) {
    auto const &record = m_builder.m_nodes[node];
    auto const detectorIndex = m_nodeIndices[node];
    m_assemblySortedDetectorIndices->emplace_back(detectorIndex);
    m_detectorPositions[detectorIndex] = Kernel::toVector3d(m_absolutePositions[node]);
    m_detectorRotations[detectorIndex] = Kernel::toQuaterniond(m_absoluteRotations[node]);
    (*m_shapes)[detectorIndex] = record.shape ? *record.shape : m_nullShape;
    (*m_scaleFactors)[detectorIndex] = Kernel::toVector3d(record.scaleFactor);
    if (record.isMonitor) {
      m_monitorIndices.emplace_back(detectorIndex);
    }
    (*m_names)[detectorIndex] = record.name;
    if (auto const &sideBySide = m_absoluteSideBySide[node]) {
      (*m_sideBySideViewPositions)[detectorIndex] = Kernel::toVector2d(*sideBySide);
    }
    return detectorIndex;
  }

  /// Non-detectors extend the arrays from the back, after the detectors.
  size_t commonRegistration(Node const node) {
    auto const &record = m_builder.m_nodes[node];
    auto const componentIndex = m_names->size();
    m_nodeIndices[node] = componentIndex;
    m_positions->emplace_back(Kernel::toVector3d(m_absolutePositions[node]));
    m_rotations->emplace_back(Kernel::toQuaterniond(m_absoluteRotations[node]));
    m_shapes->emplace_back(record.shape ? *record.shape : m_nullShape);
    m_scaleFactors->emplace_back(Kernel::toVector3d(record.scaleFactor));
    m_names->emplace_back(record.name);
    m_componentTypes->emplace_back(record.type);
    if (auto const &sideBySide = m_absoluteSideBySide[node]) {
      (*m_sideBySideViewPositions)[componentIndex] = Kernel::toVector2d(*sideBySide);
    }
    return componentIndex;
  }

  BeamlineBuilder const &m_builder;
  std::vector<size_t> m_nodeIndices;
  std::vector<V3D> m_absolutePositions;
  std::vector<Quat> m_absoluteRotations;
  std::vector<std::optional<V2D>> m_absoluteSideBySide;

  std::shared_ptr<const IObject> m_nullShape = std::make_shared<const CSGObject>();
  std::shared_ptr<std::vector<detid_t>> m_detectorIds;
  std::shared_ptr<const std::unordered_map<detid_t, size_t>> m_detectorIdToIndex;
  std::shared_ptr<std::vector<size_t>> m_assemblySortedDetectorIndices = std::make_shared<std::vector<size_t>>();
  std::shared_ptr<std::vector<size_t>> m_assemblySortedComponentIndices = std::make_shared<std::vector<size_t>>();
  std::shared_ptr<std::vector<size_t>> m_parentIndices;
  std::shared_ptr<std::vector<std::vector<size_t>>> m_children = std::make_shared<std::vector<std::vector<size_t>>>();
  std::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_detectorRanges =
      std::make_shared<std::vector<std::pair<size_t, size_t>>>();
  std::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_componentRanges =
      std::make_shared<std::vector<std::pair<size_t, size_t>>>();
  std::shared_ptr<std::vector<Eigen::Vector3d>> m_positions = std::make_shared<std::vector<Eigen::Vector3d>>();
  std::shared_ptr<Rotations> m_rotations = std::make_shared<Rotations>();
  std::vector<Eigen::Vector3d> m_detectorPositions;
  Rotations m_detectorRotations;
  std::vector<size_t> m_monitorIndices;
  std::shared_ptr<std::vector<std::shared_ptr<const IObject>>> m_shapes;
  std::shared_ptr<std::vector<Eigen::Vector3d>> m_scaleFactors;
  std::shared_ptr<std::vector<Beamline::ComponentType>> m_componentTypes =
      std::make_shared<std::vector<Beamline::ComponentType>>();
  std::shared_ptr<std::vector<std::string>> m_names;
  std::shared_ptr<std::map<size_t, Eigen::Vector2d>> m_sideBySideViewPositions =
      std::make_shared<std::map<size_t, Eigen::Vector2d>>();
  std::shared_ptr<std::map<size_t, Beamline::PixelGridComponent>> m_pixelGridComponents =
      std::make_shared<std::map<size_t, Beamline::PixelGridComponent>>();
  std::shared_ptr<std::map<size_t, Beamline::StructuredGridComponent>> m_structuredGridComponents =
      std::make_shared<std::map<size_t, Beamline::StructuredGridComponent>>();
};

BeamlineBuilder::Node BeamlineBuilder::addAssembly(Node const parent, std::string name, V3D const &position,
                                                   Quat const &rotation) {
  return add(NodeRecord{Beamline::ComponentType::Unstructured, true, std::move(name), parent, position, rotation});
}

BeamlineBuilder::Node BeamlineBuilder::addOutlineAssembly(Node const parent, std::string name, V3D const &position,
                                                          Quat const &rotation, std::shared_ptr<const IObject> shape) {
  auto const type = shape && shape->hasValidShape() ? Beamline::ComponentType::OutlineComposite
                                                    : Beamline::ComponentType::Unstructured;
  NodeRecord record{type, true, std::move(name), parent, position, rotation};
  record.shape = std::move(shape);
  return add(std::move(record));
}

BeamlineBuilder::Node BeamlineBuilder::addRectangularBank(Node const parent, std::string name, V3D const &position,
                                                          Quat const &rotation,
                                                          Beamline::PixelGridComponent pixelGrid) {
  NodeRecord record{Beamline::ComponentType::Rectangular, true, std::move(name), parent, position, rotation};
  record.pixelGrid = std::move(pixelGrid);
  return add(std::move(record));
}

BeamlineBuilder::Node BeamlineBuilder::addGridBank(Node const parent, std::string name, V3D const &position,
                                                   Quat const &rotation, Beamline::PixelGridComponent pixelGrid) {
  NodeRecord record{Beamline::ComponentType::Grid, true, std::move(name), parent, position, rotation};
  record.pixelGrid = std::move(pixelGrid);
  return add(std::move(record));
}

BeamlineBuilder::Node BeamlineBuilder::addStructuredBank(Node const parent, std::string name, V3D const &position,
                                                         Quat const &rotation,
                                                         Beamline::StructuredGridComponent structuredGrid) {
  NodeRecord record{Beamline::ComponentType::Structured, true, std::move(name), parent, position, rotation};
  record.structuredGrid = std::move(structuredGrid);
  return add(std::move(record));
}

BeamlineBuilder::Node BeamlineBuilder::addComponent(Node const parent, std::string name, V3D const &position,
                                                    Quat const &rotation) {
  return add(NodeRecord{Beamline::ComponentType::Generic, false, std::move(name), parent, position, rotation});
}

BeamlineBuilder::Node BeamlineBuilder::addObjComponent(Node const parent, std::string name, V3D const &position,
                                                       Quat const &rotation, std::shared_ptr<const IObject> shape) {
  auto const type =
      shape && shape->isFiniteGeometry() ? Beamline::ComponentType::Generic : Beamline::ComponentType::Infinite;
  NodeRecord record{type, false, std::move(name), parent, position, rotation};
  record.shape = std::move(shape);
  return add(std::move(record));
}

BeamlineBuilder::Node BeamlineBuilder::addDetector(Node const parent, std::string name, detid_t const id,
                                                   V3D const &position, Quat const &rotation,
                                                   std::shared_ptr<const IObject> shape, bool const isMonitor) {
  NodeRecord record{Beamline::ComponentType::Detector, false, std::move(name), parent, position, rotation};
  record.isDetector = true;
  record.isMonitor = isMonitor;
  record.shape = std::move(shape);
  record.id = id;
  return add(std::move(record));
}

BeamlineBuilder::Node BeamlineBuilder::add(NodeRecord record) {
  if (m_nodes.empty()) {
    if (record.parent != noParent) {
      throw std::invalid_argument("BeamlineBuilder: the first node is the root and has no parent");
    }
    if (!record.isAssembly) {
      throw std::invalid_argument("BeamlineBuilder: the root must be an assembly");
    }
  } else {
    if (record.parent == noParent) {
      throw std::invalid_argument("BeamlineBuilder: only the first node can be the root");
    }
    checkNode(record.parent);
    if (!m_nodes[record.parent].isAssembly) {
      throw std::invalid_argument("BeamlineBuilder: " + m_nodes[record.parent].name +
                                  " is not an assembly, so cannot have children");
    }
  }
  Node const node = m_nodes.size();
  if (record.parent != noParent) {
    m_children[record.parent].emplace_back(node);
  }
  m_nodes.emplace_back(std::move(record));
  m_children.emplace_back();
  return node;
}

void BeamlineBuilder::checkNode(Node const node) const {
  if (node >= m_nodes.size()) {
    throw std::out_of_range("BeamlineBuilder: no node " + std::to_string(node));
  }
}

void BeamlineBuilder::setSource(Node const node) {
  checkNode(node);
  if (m_source) {
    g_log.warning("Have already added source component to the _sourceCache.");
    return;
  }
  m_source = node;
}

void BeamlineBuilder::setSample(Node const node) {
  checkNode(node);
  if (m_sample) {
    g_log.warning("Have already added samplePos component to the _sampleCache.");
    return;
  }
  m_sample = node;
}

void BeamlineBuilder::setScaleFactor(Node const node, V3D const &scaleFactor) {
  checkNode(node);
  m_nodes[node].scaleFactor = scaleFactor;
}

void BeamlineBuilder::setSideBySideViewPosition(Node const node, V2D const &position) {
  checkNode(node);
  m_nodes[node].sideBySideViewPosition = position;
}

void BeamlineBuilder::setReferenceFrame(std::shared_ptr<const ReferenceFrame> referenceFrame) {
  m_referenceFrame = std::move(referenceFrame);
}

void BeamlineBuilder::setMetadata(std::shared_ptr<const InstrumentMetadata> metadata) {
  m_metadata = std::move(metadata);
}

void BeamlineBuilder::addParameter(Node const node, std::shared_ptr<Parameter> parameter) {
  checkNode(node);
  if (!parameter) {
    throw std::invalid_argument("BeamlineBuilder: a parameter cannot be null");
  }
  m_parameters.emplace_back(node, std::move(parameter));
}

void BeamlineBuilder::addLogfileParameter(Node const node, std::string const &cacheKey,
                                          std::shared_ptr<const XMLInstrumentParameter> parameter) {
  checkNode(node);
  if (!parameter) {
    throw std::invalid_argument("BeamlineBuilder: a logfile parameter cannot be null");
  }
  m_logfileParameters.emplace_back(node, cacheKey, std::move(parameter));
}

size_t BeamlineBuilder::size() const { return m_nodes.size(); }

/// True if the node is an assembly of `columns` columns, each holding `pixels` detectors.
bool BeamlineBuilder::isPixelGrid(Node const node, int const columns, int const pixels) const {
  auto const &columnNodes = m_children[node];
  if (columns <= 0 || pixels <= 0 || columnNodes.size() != static_cast<size_t>(columns)) {
    return false;
  }
  return std::ranges::all_of(columnNodes, [this, pixels](Node const column) {
    auto const &pixelNodes = m_children[column];
    return m_nodes[column].isAssembly && pixelNodes.size() == static_cast<size_t>(pixels) &&
           std::ranges::all_of(pixelNodes, [this](Node const pixel) { return m_nodes[pixel].isDetector; });
  });
}

/// Banks are read as grids of pixels (see ComponentInfo::quadrilateralComponent), so each must hold its grid.
void BeamlineBuilder::checkBanks() const {
  for (Node node = 0; node < m_nodes.size(); ++node) {
    auto const &record = m_nodes[node];
    bool valid = true;
    if (record.structuredGrid) {
      valid = isPixelGrid(node, record.structuredGrid->nX, record.structuredGrid->nY);
    } else if (record.pixelGrid) {
      auto const &grid = *record.pixelGrid;
      if (record.type == Beamline::ComponentType::Grid && grid.nZ > 0) {
        auto const &layers = m_children[node];
        valid = layers.size() == static_cast<size_t>(grid.nZ) &&
                std::ranges::all_of(layers, [this, &grid](Node const layer) {
                  return m_nodes[layer].isAssembly && isPixelGrid(layer, grid.nX, grid.nY);
                });
      } else {
        // A rectangular bank is flat
        valid = (record.type != Beamline::ComponentType::Rectangular || grid.nZ == 0) &&
                isPixelGrid(node, grid.nX, grid.nY);
      }
    }
    if (!valid) {
      throw std::invalid_argument("BeamlineBuilder: bank " + record.name +
                                  " does not hold the grid of pixels it describes");
    }
  }
}

BeamlineRef BeamlineBuilder::build() const {
  if (m_nodes.empty()) {
    throw std::logic_error("BeamlineBuilder: there is no root node to build");
  }
  checkBanks();
  Layout layout(*this);

  auto parameterInfo = std::make_shared<ParameterInfo>();
  for (auto const &[node, parameter] : m_parameters) {
    parameterInfo->add(layout.index(node), parameter);
  }
  for (auto const &[node, cacheKey, parameter] : m_logfileParameters) {
    parameterInfo->addLogfileParameter(layout.index(node), cacheKey, parameter);
  }

  auto beamlineComponentInfo = layout.componentInfo();
  auto beamlineDetectorInfo = layout.detectorInfo();
  beamlineComponentInfo->setDetectorInfo(beamlineDetectorInfo.get());
  auto componentInfo = std::make_unique<ComponentInfo>(std::move(beamlineComponentInfo), layout.shapes(), parameterInfo,
                                                       m_referenceFrame);
  auto detectorInfo = std::make_unique<DetectorInfo>(std::move(beamlineDetectorInfo), layout.detectorIds(),
                                                     layout.detectorIdToIndex(), parameterInfo, m_referenceFrame);
  return makeBeamlineRef(std::move(componentInfo), std::move(detectorInfo), m_metadata);
}

BeamlineRef makeBeamlineRef(std::unique_ptr<ComponentInfo> componentInfo, std::unique_ptr<DetectorInfo> detectorInfo,
                            std::shared_ptr<const InstrumentMetadata> metadata) {
  // The ComponentInfo points at its DetectorInfo, so one owner keeps both
  struct Infos {
    std::unique_ptr<ComponentInfo> componentInfo;
    std::unique_ptr<DetectorInfo> detectorInfo;
  };
  auto const infos = std::make_shared<Infos>(std::move(componentInfo), std::move(detectorInfo));
  return {std::shared_ptr<const ComponentInfo>(infos, infos->componentInfo.get()),
          std::shared_ptr<const DetectorInfo>(infos, infos->detectorInfo.get()), std::move(metadata)};
}

} // namespace Mantid::Geometry
