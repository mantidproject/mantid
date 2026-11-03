(v2.5.0-vates)=

# Vates

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_5_release/NonOrthogonalProjection.webp
---
class: screenshot
width: 415px
align: right
alt: NonOrthogonalProjection
---
```

## New Features

Vates is our ongoing effort to provide multidimensional workspace MDWorkspace processing and 3D visualisation techniques directly in Mantid. For more details see here

## ParaView 3D Visualisation

This release incorporates new features to the 3D visualisation capability within Mantid. The new features are those provided in work done by the Kitware Corporation and folded into the latest ParaView release. The summary of the new features are:

- Ability to provide axis labels from datasets
- Ability to provide "time" label from datasets
- Allow override of axis extents for displaying original extents
- Provide methodology for overriding axis basis for non-orthogonal datasets
- Porting of the MultiSlice view into ParaView proper
- QuadView (ThreeSlice) plugin that incorporates coordinate readout and dynamic slice repositioning

Many of the new features (axis labels, override axis extents, non-orthogonal axis basis) are on display in the main release notes image here on this page. This image was produced using the Mantid VATES plugins in ParaView. However, these features are also folded into an upgraded VatesSimpleInterface. The new help page for the VatesSimpleInterface shows pictures of the new MultiSlice and ThreeSlice views.

### Other Important Items

- ScaleWorkspace plugin uses axis bounds override to display original extents
- ParaView reader for MDHistoWorkspace NeXus files
- Metadata written/read from MD files for non-orthogonal viewing

### Important Fixes

- {ref}`MergeMD <algm-MergeMD>` correctly aggregates all experiment information into the merged workspace

### Technique specific VATES Improvements

For Diffraction specific VATES improvements see the release subsection notes {ref}`here <v2.5.0-diffraction>`
