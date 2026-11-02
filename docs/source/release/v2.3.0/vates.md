(v2.3.0-vates)=

# Vates

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## New Features

- Expose number of MDEvents in MD workspace to python
- Lorentz correction in ConvertToMD
- Added Chopper jitter to FermiChopper model
- Added Chopper jitter to TobyFitResolution model

## Quantification

In version 2.2 the quantification portion of the Vates framework was {ref}`introduced <v2.2.0-vates>`. This was a "first cut" version that included the resolution model from [TobyFit](http://tobyfit.isis.rl.ac.uk/Main_Page) and a single foreground model that would be used for verification purposes. More details here.

Version 2.3 has included some fixes to the framework surrounding conversion to Q-space along with dealing with the UB matrix within the foreground model. These fixes have brought the output of a simulation from Mantid much closer to that produced by Tobyfit (see image below). However, verification & improvement is an ongoing task and this area remains in active development.

```{figure} ../../images/2_3_release/MantidVsTobyFit.webp
---
class: screenshot
width: 1000px
align: right
---
Slices from simulation of MERLIN instrument resolution convolved with a Strontrium122 foreground model using Mantid(left) & TobyFit(right). The horizontal line shows the expected alignment of the peaks given that the sample itself was misaligned.
```

## Defects Fixed

- SaveMD crashes Mantid when saving large MD files
- ConvertToMD loses information about grouping
- ConvertToMD slow peformance
- SliceMD not working on file-based Workspaces
- SliceViewer not launchable from Vates Simple Interface
- SliceViewer colour range incorrectly calculated
- Wrong axis plotted in SliceViewer
- Rebinning with collapsed dimensions gives the wrong t-axis value in ParaView

## Scoped Features

We have been working in collaboration with [Kitware](http://www.kitware.com/) to design new [ParaView](http://www.paraview.org/) features for Mantid. The following features will arrive in the next version of Paraview, and will then be utilised in the next release of Mantid.

### New Views

- Embedded SliceView, based on MultiSlice in the VSI and Slicomatic
- Embedded QuadView, based on our home-grown ThreeSlice view and the SliceViewer

### Support for Natural Coordinate Systems

- Non-orthogonal axes
- Custom axis labels and units
- Custom ranges for axis
- QuadView reports cross-hair location in natural coordinates as well as scalar intensities

### Other Collaborated Improvements

- Improved colour range controls
- Greater variety of colour maps
- Stability against our current target QT 4.7 version

## Other Features

- System Tests for Vates Quantification
- System Tests LoadMD
