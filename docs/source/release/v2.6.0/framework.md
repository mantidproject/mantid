(v2.6.0-framework)=

# Framework

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Important News

```{figure} ../../images/2_5_release/PythonRetirementTimeline.webp
---
class: screenshot
width: 500px
align: right
---
Python Retirement Timeline.
```

### Retirement of Mantid Python v1

- We have been supporting two versions of the Python API since Mantid v2.0. Continuing to maintain both versions restricts further development and doubles the effort in maintaining the code.
- Therefore we are phasing out v1 of the API “from mantidsimple”, in favor of it's replacement v2 “from mantid.simpleapi”.
  - This release, v2.6, will run both versions, but use of v1 of the python API will create a deprecation warning. Please do not ignore this warning as scripts that use this will NOT work in the next release of Mantid.
  - Release 3.0 will only support v2 of the python API and will not run v1 scripts.
- How to Update your scripts to v2
  - Upload the scripts to the script repository (we have extended the deadline to the 2nd of August). The development team will then migrate all scripts in the repository, they will contact the owner for testing and verification if necessary.
  - Try {ref}`migrate1to2.py <v2.1.0-python>` the automatic migration script.
  - Look at the python [changes notes](http://download.mantidproject.org/docs/current-release/python/html/changes.html) and try to update them yourselves.
  - Contact the Development Team for help.

## New Features

### Loading Data

- The performance of the Load Algorithm has been speeded up, most notably for Nexus files, but with some benefit for all files.
- You can now use Load to load an instrument_definition.xml file as an empty instrument.
- Improved performance when loading Event Nexus files with small numbers of banks defined in the file.
- Load can now load image data stored in vtk files, to bring externally processed data into the 3D visualisation tools. This algorithm can output either a MDHistogrammed workspace or, an adaptively binned MDEventWorkspace which is much more efficient for sparse data.
- McStas Simulations using the monitor_nD detector can now be loaded as event workspaces using LoadMcStasEventNexus.
- LoadILL can now load data from IN4,5 & 6 and D33.
- LoadPSI has been added to allow data from SINQ to be loaded, although at present it only supports FOCUS.
- LoadLLB has been added to allow data from LLB to be loaded, although at present it only supports Mibemol.

```{figure} ../../images/2_5_release/MERLINCalibration.webp
---
class: screenshot
width: 500px
align: right
---
Merlin with the centre door calibrated.
```

### Tube Calibration

- The interface for working with Tube Calibration has been much improved, and is ready for production use.
- Extensive documentation available in this link, and examples on how to use in this link
- Stored instrument details are now correctly loaded from evrnt based nexus files as well as Histogram ones if present. If the instrument definition is not found the xml definition in the instrument directory will be used.

### Curve Fitting

- If your data includes NaNs, infinities and points with 0 error you can now choose to ignore these in your fit by selecting the "Ignore invalid data" option in the Fit Browser.
- A bug in StretchExp that affected fitting around x=0 has been fixed.
- CubicSpline function has been added to the background category.

### Live Data

- For facilities that support this (at present ADARA enadbled instruments at ORNL) you can now start a live data stream from the start of the run even when part way through.

### New and Changed Algorithms

- {ref}`IntegrateByComponent <algm-IntegrateByComponent>` - Allows you to integrate the data by bank / module or door etc. by specifying how many levels above the detector you wish to integrate at.
- Binary algorithms (Plus, Divide etc) - The error messages it the two workspaces are not compatible have been improved.
- TOFExtinction - Extinction correction for single crystal peaks.
- {ref}`LoadLogPropertyTable <algm-LoadLogPropertyTable>` - Will load a table of all of the values of a log across a range of files.
- {ref}`ExtractMaskToTable <algm-ExtractMaskToTable>` - Extracts the masking information to a table workspace.
- {ref}`ConvertAxisByFormula <algm-ConvertAxisByFormula>` - Converts the X or Y axis of a MatrixWorkspace via a user defined math formula.
- {ref}`ThresholdMD <algm-ThresholdMD>` - Allows a less than or greater then threshold to be applied to MDHistogram Workspaces, for the purposes of imaging.
- {ref}`SetSampleMaterial <algm-SetSampleMaterial>` - Several improvements have been made.
- {ref}`SmoothNeighbours <algm-SmoothNeighbours>` - Now detects if the instrument is based on rectangular detectors and adjusts the parameters appropriately.
- {ref}`SplineSmoothing <algm-SplineSmoothing>` - Smooths a MatrixWorkspace using a cubic spline.
- {ref}`SplineInterpolation <algm-SplineInterpolation>` - Interpolate one MatrixWorkspace onto another MatrixWorkspace using a cubic spline.
- ModeratorTzero - Has been renamed {ref}`ModeratorTzeroLinear <algm-ModeratorTzeroLinear>`.

### Removed Algorithms

- RefEstimates - Was used in a REFM demo but not used in production.
- EQSANSTransmission - This old method of computing the transmission has not been in use for years.
- LoadNISTSANS - An old demo algorithm that was never used in production.

### Algorithms to be Removed in the next release

Please using to moving the algorithms indicated

- CaltoDspacemap -> LoadCalFile, then SaveDspacemap
- DspacemaptoCal -> LoadDspacemap
- ExtractMasking -> ExtractMask
- FilterEventsHighFrequency -> Prototype only, no replacement
- GhostCorrection -> No replacement
- Unwrap -> UnwrapMonitor
- Gaussian1D -> Fit
- Linear -> Fit
- GetMaskedDetectors -> ExtractMask
- LoadDAE -> StartLiveData
- LoadLogsFromSNSNexus -> LoadNexusLogs
- LoadMaskingFile -> LoadMask
- LoadSNSEventNexus -> LoadEventNexus
- LoadSNSNexus -> LoadTOFRawNexus
- SaveDetectorMasks -> SaveMask
- BinToMDHistoWorkspace -> BinMD
- ConvertToMDEvents -> ConvertToMD
