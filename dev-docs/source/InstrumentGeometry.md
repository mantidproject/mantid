(InstrumentGeometry)=

# Instrument Geometry

```{contents}
---
local:
---
```

## Overview

Mantid has two models of an instrument:

- the **Instrument 2.0 layers**, `SpectrumInfo`, `DetectorInfo` and `ComponentInfo`, which store instrument information in flat arrays accessed by index, together with `InstrumentMetadata` for information about the instrument as a whole, and
- the **legacy component tree**, `Instrument` and its `Component` objects, which is still used to build an instrument and by older code.

New code should use the Instrument 2.0 layers.
How to use them, and how to convert code that uses the legacy tree, is described in {ref}`mantid:InstrumentAccessLayers`.
This page describes how the geometry is built and stored inside the framework.

## How the layers fit together

The layers are split across three libraries:

`Beamline`
: `Beamline::ComponentInfo`, `Beamline::DetectorInfo` and `Beamline::SpectrumInfo` hold the data.
Each component has an index, and the positions, rotations, parent indices and child indices are stored in arrays by that index.
The position and rotation arrays are copy-on-write, so workspaces that share an instrument share the arrays until one of them moves a component.
Positions and rotations can also be stored per scan interval, for instruments that move detectors during a measurement.

`Geometry`
: `Geometry::ComponentInfo` and `Geometry::DetectorInfo` wrap the `Beamline` classes and add what the `Beamline` library does not know about:
the shape of each component, detector IDs, the mapping back to the legacy tree's `ComponentID` and the instrument parameters (`ParameterInfo`).
`Geometry::InstrumentMetadata` holds the information about the whole instrument, see [Instrument metadata](#instrument-metadata).

`API`
: `API::SpectrumInfo` maps workspace spectra to detector indices.

`ExperimentInfo` creates the `Geometry` layers the first time they are needed, by walking the legacy instrument tree with `InstrumentVisitor::makeWrappers`, and owns them from then on.
Use `componentInfo()` and `detectorInfo()` for read access and `mutableComponentInfo()` and `mutableDetectorInfo()` to change positions, rotations, masking or parameters.

### Indexes and IDs

Code should work with indexes, which are positions in the arrays, and use IDs only for user input and output:

- the component index of a detector is equal to its detector index, so the first components are the detectors,
- the root of the instrument is always the last component index, `ComponentInfo::root()`, and
- `ComponentInfo::indexOf(componentID)` and `DetectorInfo::indexOf(detectorID)` convert IDs to indexes, but involve a map lookup, so avoid calling them inside loops.

## Instrument metadata

`Geometry::InstrumentMetadata` holds the information that belongs to the instrument as a whole rather than to any component, so it does not fit the index-based layers:

- the dates the instrument definition is valid from and to,
- the name of the instrument definition file and its XML text,
- the default view and view axis for the instrument viewer, and
- for indirect geometry instruments, the separate physical instrument, if the definition has one.

`ParameterMap::buildInstrumentMetadata` creates it from the legacy instrument when the instrument is set on a workspace, and `ExperimentInfo` shares ownership of it in the same way as the `ComponentInfo` and `DetectorInfo`.
Use `ExperimentInfo::instrumentMetadata()` instead of the matching `Instrument` methods:

| Legacy `Instrument` method                | Replacement                                              |
| ----------------------------------------- | -------------------------------------------------------- |
| `getValidFromDate()` / `getValidToDate()` | `instrumentMetadata().validFromDate()` / `validToDate()` |
| `getFilename()` / `getXmlText()`          | `instrumentMetadata().filename()` / `xmlText()`          |
| `getDefaultView()` / `getDefaultAxis()`   | `instrumentMetadata().defaultView()` / `defaultAxis()`   |
| `getPhysicalInstrument()`                 | `instrumentMetadata().physicalInstrument()`              |

The filename, XML text, default view and default view axis can be changed with the `set` methods on `InstrumentMetadata`; the validity dates and the physical instrument are fixed when the instrument is loaded.
The physical instrument is still a legacy `Instrument`, because replacing it needs its own nested `ComponentInfo` and `DetectorInfo`.

Python does not expose the `InstrumentMetadata` object itself.
Instead `ExperimentInfo`, and so every workspace, has `instrumentValidFromDate()`, `instrumentValidToDate()`, `instrumentFilename()`, `instrumentXmlText()`, `instrumentDefaultView()` and `instrumentDefaultAxis()`.

## Shapes

Most instruments have thousands of detectors but only a few distinct shapes, e.g. a MERLIN pixel is one of 250,000 identical cylinders.
Mantid therefore defines the shape of a component separately from where it is placed:

- Each distinct shape is one `IObject`, either a `CSGObject` (constructive solid geometry, built from surfaces such as planes and cylinders) or a `MeshObject` (a triangle mesh).
  `ShapeFactory` creates shapes from the XML described in {ref}`mantid:HowToDefineGeometricShape`.
- A shape is defined in its own coordinate frame and knows nothing about the instrument.
  It should be centred on the origin, otherwise it appears to move when the component is rotated.
- `Geometry::ComponentInfo` holds a shared pointer to the shape of each component, so identical components share one shape object.
  `ComponentInfo::shapeToComponentIndices()` returns the components that use each shape.
- Shape operations such as `ComponentInfo::solidAngle` or bounding boxes take a component index, transform the question into the shape's own frame using the component's position, rotation and scale factor, and ask the shape.

To display an instrument, the shapes are triangulated.
Simple shapes are triangulated directly; complex CSG shapes use OpenCascade when Mantid is built with `ENABLE_OPENCASCADE`.
Because this is slow, the triangulated shapes are cached in a `.vtp` file when an instrument definition is first loaded.

## Code layout

`Framework/Beamline`
: The `Beamline` classes described above.

`Framework/Geometry`
: The interfaces used throughout the framework, e.g. `IComponent` and `IDetector`, are at the top level.
The subdirectories are:

- `Instrument`: the `ComponentInfo` and `DetectorInfo` wrappers, the legacy tree classes, the instrument definition parser and the instrument parameters,
- `Objects`: shapes (`IObject`, `CSGObject`, `MeshObject`), `ShapeFactory`, tracks and ray tracing,
- `Surfaces`: the surfaces used to build CSG shapes,
- `Rendering`: triangulation, OpenGL rendering and the `.vtp` cache,
- `Math`: matrices, algebra and other mathematics helpers,
- `Crystal`: unit cells, point and space groups and other crystallography,
- `MDGeometry`: dimensions and coordinate transformations for multi-dimensional workspaces.

## The legacy component tree

Instrument definitions are still loaded into the legacy tree first, and older code still uses it, so it is worth knowing how it works:

- `Component`, `CompAssembly` and `ObjComponent` form a tree using the Composite pattern (see {doc}`Architecture`).
  `ObjComponent` combines a component with a shape, and `Detector` adds a detector ID.
- Each component stores its position and rotation relative to its parent, and `getPos()` and `getRotation()` work out the absolute values.
  For the instrument of a workspace these now read from `ComponentInfo`.
- The instrument tree is shared between workspaces, so values that differ between workspaces, such as moved components and instrument parameters, cannot be stored on it.
  They were stored in the workspace's `ParameterMap`, which now delegates to `ParameterInfo` and `ComponentInfo`.

Do not add new uses of the legacy tree.
In particular, `DetectorInfo::detector` and `DetectorInfo::getDetector` have been deprecated, and loops over `Instrument::getChildren` should become loops over `ComponentInfo::componentsInSubtree`.
