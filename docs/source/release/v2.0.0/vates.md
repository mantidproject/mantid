(v2.0.0-vates)=

# Vates

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Mantid Vates 2.0

## Multi-Dimensional Algorithms

- ConvertToMDEvents (New)
- {ref}`QueryMDWorkspace <algm-QueryMDWorkspace>` (New)
- {ref}`LoadSQW <algm-LoadSQW>`(Improved)
- {ref}`MergeMD <algm-MergeMD>` (New)
- {ref}`ConvertToDiffractionMDWorkspace <algm-ConvertToDiffractionMDWorkspace>` (Improved)

## Vates Workflow Algorithms

- {ref}`SmoothNeighbours <algm-SmoothNeighbours>` (Improved)
- {ref}`FindSXPeaks <algm-FindSXPeaks>` (New)
- {ref}`SaveNexus <algm-SaveNexus>` and {ref}`LoadNexus <algm-LoadNexus>` work on PeaksWorkspaces (Improved)
- {ref}`CentroidPeaksMD <algm-CentroidPeaksMD>` (New)
- {ref}`PredictPeaks <algm-PredictPeaks>` (New)
- FindUBUsingMinMaxD (Improved)

## Other Noteable Features

- Binary and Unary Algorithms for MDHistoWorkspaces(New)
- Plotting for MDWorkspaces (New)
- Overplotting of MDWorkspace cuts (New)
- Vates Simple Interface rendering of MDHistoWorkspaces (New)
- Peak workspace overplotting for MDHistoWorkspaces in 3D (New)
- Faster rendering of MDWorkspaces in 3D initial view (Improved)
- Nearest Neighbours searching much faster (Improved)
- Paraview 3D viewer communicates with 2D Slice Viewer (New)
- Python interface for real-time control of SliceViewer and LineViewer (New)

## Slice Viewer

The SliceViewer shows 2D slices of multi-dimensions workspaces, allowing the user to choose which dimensions to visualize, at what points.

```{figure} ../../images/2_0_release/SliceViewer_SlicePoint_Animation.webp
---
class: screenshot
width: 611px
align: right
alt: SliceViewer SlicePoint Animation
---
```

Right-click on an arrow in the multi-slice view in the VatesSimpleInterface to open the SliceViewer at the same slice.

## Line Viewer

The LineViewer is used within the SliceViewer to integrate 1D lines through multi-dimensional data sets:

```{figure} ../../images/2_0_release/LineViewerAnimation.webp
---
class: screenshot
width: 1091px
align: right
alt: LineViewerAnimation
---
```

## Vates Simple Interface

- Vates Simple Interface

### Three Slice

```{figure} ../../images/2_0_release/VSI-Threeslice.webp
---
class: screenshot
width: 700px
align: right
alt: VSI-Threeslice
---
```

### Multi Slice

```{figure} ../../images/2_0_release/Multislice_Animation.webp
---
class: screenshot
width: 700px
align: right
alt: Multislice Animation
---
```

### Peaks Overlay

```{figure} ../../images/2_0_release/Vates_Simple_Interface_Splatter_with_PeakOverlay.webp
---
class: screenshot
width: 700px
align: right
alt: Vates Simple Interface Splatter with PeakOverlay
---
```

### Orthographic Mode

```{figure} ../../images/2_0_release/Ortho_View_Animation.webp
---
class: screenshot
width: 700px
align: right
alt: Ortho View Animation
---
```

## Multi-Dimensional Creation GUI

Create Multi-Dimensional Workspace GUI

- Converts N Matrix Workspaces into N Multi-Dimensional Workspaces.
- Optional merging into a single super Multi-Dimensional Workspace.
- Can work either directly on your processed files, or on workspaces already in the MantidPlot Workspace List.
- Allows modifications to individual workspaces without leaving the GUI, such as adding missing log values.

```{figure} ../../images/2_0_release/Createmdworkspacegui.webp
---
class: screenshot
width: 917px
align: right
alt: Createmdworkspacegui
---
```
