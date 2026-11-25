// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidBeamline/ComponentType.h"
#include "MantidBeamline/PixelGridComponent.h"
#include "MantidBeamline/StructuredGridComponent.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/BeamlineRef.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
class IObject;
class InstrumentMetadata;
class Parameter;
class ReferenceFrame;
class XMLInstrumentParameter;

/** BeamlineBuilder : builds ComponentInfo and DetectorInfo directly, without a legacy component tree.

  The instrument is described as a tree of nodes. Each node is added under a parent node that was added before
  it, and has a position and rotation relative to that parent. The first node has no parent and is the root.
  Nodes are referred to by the handle returned when they are added. Handles are not component indices: build()
  lays the components out as InstrumentVisitor does.
  - Detectors come first, ordered by detector ID, so a detector's component index is its detector index.
  - The other components follow in post-order (each after its children, the root last).
  - Children are listed in the order they were added.
 */
class MANTID_GEOMETRY_DLL BeamlineBuilder {
public:
  /// A handle to a node, as returned when the node is added.
  using Node = size_t;
  /// The parent of the root node.
  static constexpr Node noParent = std::numeric_limits<size_t>::max();

  /// An assembly of other components.
  Node addAssembly(Node const parent, std::string name, Kernel::V3D const &position = Kernel::V3D(),
                   Kernel::Quat const &rotation = Kernel::Quat());
  /// An assembly with an outline shape, as ObjCompAssembly. It is an outline composite if the shape is valid.
  Node addOutlineAssembly(Node const parent, std::string name, Kernel::V3D const &position,
                          Kernel::Quat const &rotation, std::shared_ptr<const IObject> shape);
  /// A rectangular bank. Its pixels are added under it as for any assembly: nX columns, each of nY detectors.
  Node addRectangularBank(Node const parent, std::string name, Kernel::V3D const &position,
                          Kernel::Quat const &rotation, Beamline::PixelGridComponent pixelGrid);
  /// A 3D grid bank. Its pixels are added under it as for any assembly: nZ layers (if nZ > 0), each of nX columns of
  /// nY detectors.
  Node addGridBank(Node const parent, std::string name, Kernel::V3D const &position, Kernel::Quat const &rotation,
                   Beamline::PixelGridComponent pixelGrid);
  /// A rectangular bank holding the pixels RectangularDetector::initialize makes from pixelGrid, all of pixelShape:
  /// columns and pixels named, placed and numbered as GridDetector does. nZ must be 0. The grid's minimum and
  /// maximum detector IDs are filled in. Returns the bank.
  Node addRectangularBank(Node const parent, std::string name, Kernel::V3D const &position,
                          Kernel::Quat const &rotation, Beamline::PixelGridComponent pixelGrid,
                          std::shared_ptr<const IObject> const &pixelShape);
  /// A 3D grid bank holding the pixels GridDetector::initialize makes from pixelGrid, as addRectangularBank does.
  Node addGridBank(Node const parent, std::string name, Kernel::V3D const &position, Kernel::Quat const &rotation,
                   Beamline::PixelGridComponent pixelGrid, std::shared_ptr<const IObject> const &pixelShape);
  /// A structured bank. Its pixels are added under it as for any assembly: nX columns, each of nY detectors.
  Node addStructuredBank(Node const parent, std::string name, Kernel::V3D const &position, Kernel::Quat const &rotation,
                         Beamline::StructuredGridComponent structuredGrid);
  /// A component without a shape, which has no children.
  Node addComponent(Node const parent, std::string name, Kernel::V3D const &position = Kernel::V3D(),
                    Kernel::Quat const &rotation = Kernel::Quat());
  /// A component with a shape, which has no children. It is infinite unless the shape is finite, as ObjComponent.
  Node addObjComponent(Node const parent, std::string name, Kernel::V3D const &position, Kernel::Quat const &rotation,
                       std::shared_ptr<const IObject> shape);
  /// A detector, which has no children. Detector IDs must be unique.
  Node addDetector(Node const parent, std::string name, detid_t const id, Kernel::V3D const &position,
                   Kernel::Quat const &rotation, std::shared_ptr<const IObject> shape, bool const isMonitor = false);

  /// The source. As in the legacy instrument, only the first source set is kept; later ones are warned about.
  /// A copy of the component at `index` of an existing beamline, and of everything under it, added under `parent`.
  /// Each copied component keeps its position, rotation and side-by-side view position relative to its own parent,
  /// as a cloned legacy component does. Returns the copy.
  Node addCopy(Node const parent, ComponentInfo const &componentInfo, DetectorInfo const &detectorInfo,
               size_t const index);

  void setSource(Node const node);
  /// The sample. As in the legacy instrument, only the first sample set is kept; later ones are warned about.
  void setSample(Node const node);
  /// The node's scale factor. It is (1, 1, 1) unless set.
  void setScaleFactor(Node const node, Kernel::V3D const &scaleFactor);
  /// The node's side-by-side view position, relative to its parent's.
  void setSideBySideViewPosition(Node const node, Kernel::V2D const &position);
  void setReferenceFrame(std::shared_ptr<const ReferenceFrame> referenceFrame);
  void setMetadata(std::shared_ptr<const InstrumentMetadata> metadata);

  /// Every build shares the parameter objects: changing one changes it in every beamline built.
  void addParameter(Node const node, std::shared_ptr<Parameter> parameter);
  void addLogfileParameter(Node const node, std::string const &cacheKey,
                           std::shared_ptr<const XMLInstrumentParameter> parameter);

  /// The number of nodes added.
  size_t size() const;

  /// Lay out the nodes and make the Infos. The builder can be built again; the builds share their parameters.
  /// Throws if a detector ID is used twice, or a bank does not hold the grid of pixels it describes.
  BeamlineRef build() const;

private:
  struct NodeRecord {
    NodeRecord(Beamline::ComponentType const type, bool const isAssembly, std::string name, Node const parent,
               Kernel::V3D const &position, Kernel::Quat const &rotation)
        : type(type), isAssembly(isAssembly), name(std::move(name)), parent(parent), position(position),
          rotation(rotation) {}
    Beamline::ComponentType type;
    bool isAssembly;
    bool isDetector{false};
    bool isMonitor{false};
    std::string name;
    Node parent;
    Kernel::V3D position;
    Kernel::Quat rotation;
    Kernel::V3D scaleFactor{1, 1, 1};
    /// Unset for components without a shape, which get an empty shape.
    std::optional<std::shared_ptr<const IObject>> shape;
    detid_t id{0};
    std::optional<Kernel::V2D> sideBySideViewPosition;
    std::optional<Beamline::PixelGridComponent> pixelGrid;
    std::optional<Beamline::StructuredGridComponent> structuredGrid;
  };
  class Layout;

  Node add(NodeRecord record);
  void checkNode(Node const node) const;
  bool isPixelGrid(Node const node, int const columns, int const pixels) const;
  void addGridPixels(Node const bank, std::shared_ptr<const IObject> const &pixelShape);
  void checkBanks() const;

  std::vector<NodeRecord> m_nodes;
  std::vector<std::vector<Node>> m_children;
  std::optional<Node> m_source;
  std::optional<Node> m_sample;
  std::shared_ptr<const ReferenceFrame> m_referenceFrame;
  std::shared_ptr<const InstrumentMetadata> m_metadata;
  std::vector<std::pair<Node, std::shared_ptr<Parameter>>> m_parameters;
  std::vector<std::tuple<Node, std::string, std::shared_ptr<const XMLInstrumentParameter>>> m_logfileParameters;
};

} // namespace Geometry
} // namespace Mantid
