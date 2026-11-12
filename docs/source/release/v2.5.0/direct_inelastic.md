(v2.5.0-direct_inelastic)=

# Direct Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_5_release/TIBsuggestion.webp
---
class: screenshot
width: 415px
align: right
---
Suggested time independent range for CNCS tries to avoid prompt pulse.
```

## New Features

ConvertToMD has the ability to calculate momentum transfer in laboratory frame, sample frame, or reciprocal lattice frame. The algorithm can decide automatically which frame to choose, based on the information available on the initial workspace. A UB matrix and goniometer is necessary to get Q in the reciprocal lattice frame (in inverse angstroms or reciprocal lattice units), a goniometer only will yield Q in the sample frame, otherwise the calculated Q is in the laboratory frame.

## Autoreduction helpers

Several algorithms were added or modified, in order to improve automatic reduction of the direct inelastic neutron data:

1. Algorithms to suggest best range for time independent background for CNCS and HYSPEC
1. Improvements for GetEi algorithm
1. Ability to capture Mantid logging information and errors on different channels

## Other improvements and updates

1. Move reduction scripts to version 2 of the Python API
1. Improved information stored in MD workspaces allows for non-orthogonal axes in visualization (see {ref}`Vates <v2.5.0-vates>`)

## Bugfixes

1. MedianDetectorTest used to fail when the median was performed on a subcomponent of the instrument, when all detectors of that subcomponent were masked
1. Time independent background failing for certain default parameters in the DGSReduction GUI
1. Improved Nexus standards compliance for NXSPE files
1. DirectConvertToEnergy bug fixes
