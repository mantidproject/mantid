(v2.6.0-diffraction)=

# Diffraction

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Single Crystal Diffraction

### New Single Crystal Algorithms

- Filter Peaks workspaces by operators on the Miller indices {ref}`FilterPeaks <algm-FilterPeaks>`
- Finds chi and omega rotations for two runs whose sample positions differ by only their phi angles {ref}`GoniometerAnglesFromPhiRotation <algm-GoniometerAnglesFromPhiRotation>`
- Displays offsets of h,k,and l from an integer along with bank and run number {ref}`ShowPeakHKLOffsets <algm-ShowPeakHKLOffsets>`
- New single crystal peak integration option to integrate cylinder instead of sphere and fit profile along length of cylinder {ref}`IntegratePeaksMD <algm-IntegratePeaksMD>`
- Load will now load a single crystal peaks or integrate file
- New workspace containing number of peaks integrated to I/sigI levels is output to help determine radius for integrating single crystal peaks {ref}`PeakIntensityVsRadius <algm-PeakIntensityVsRadius>`

### Handling large PeaksOverlays for the SliceViewer

- PeaksViewer performance profiling
- Change the way that Peaks Workspaces are over-plotted in the PeaksViewer to provide better handling for numbers of peaks of the order 10^3 or greater
- Create new Parallel-processing algorithms {ref}`PeaksInRegion <algm-PeaksInRegion>` to provide very fast calculations as a back-end for the PeaksViewer GUI
- New Parallel-proccesing algorithm called {ref}`PeaksOnSurface <algm-PeaksOnSurface>`, which is more flexible than PeaksInRegion, but not yet utilised by the GUI
- New widget layout for the PeaksViewer, more compact, and works better cross-platform

### Python Additions

- First cut at calculating in-plane angles. Peformed entirely via a python wrapper class.
- PeaksWorkspaces can be overplotted via python

```
  #################
  svw = plotSlice('QLab')
  sv = svw.getSlicer()
  # Over-plot two peaks workspaces
  sv.setPeaksWorkspaces(['peaks_qLab', 'peaks_qLab_Integrated'])
  # Clear the peaks workspaces
  sv.clearPeaksWorkspaces()
```

### Noteworthy Fixes

- Fixed crash in {ref}`FindPeaksMD <algm-FindPeaksMD>`
- Fixed memory leak in CalibrateRectangularDetectors
- Find and store errors in reciprocal lattice parameters (SCD) when found using least squares optimisation
- Address usability issues in the {ref}`OptimizeCrystalPlacement <algm-OptimizeCrystalPlacement>` Algorithm

## Powder Diffraction

### New Algorithms

- RemoveLowResTOF is able to save the low resolution TOF neutron events, which were discarded previously, to an addition workspace.
- SaveGSS and LoadGSS can use spectrum IDs as bank ID.
- Low resolution TOF can be reduced as data in a new diffractometer bank.

### Noteworthy Improvements

- PDF calculator
  - calculate Q(S(Q)-1
  - Improve PDFFourierTransform
- FindPeaks
  - Algorithm **FindPeaks** is refactored and improved with robustness
  - GetDetOffsetMultPeaks is benefited from improvement on FindPeaks. For example, calibration of POWGEN's detector is significantly better than before. (see graph)

### Instrument Geometry

- POWGEN's information for the geometry is updated as survey and alignment has new values for instrument geometry.

### Python Additions

- Material is exposed to Python
