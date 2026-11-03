(v2.4.0-diffraction)=

# Diffraction

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

The {ref}`Vates release notes <v2.4.0-vates>` contain other information about single crystal diffraction changes.

```{figure} ../../images/2_1_release/ImageViewer.jpg
---
class: screenshot
width: 400px
align: right
alt: ImageViewer
---
```

## New Features

## Powder Diffraction

- The algorithms used within SNSPowderDiffraction have been refactored to allow for easier use in other contexts. {ref}`AlignAndFocusPowder <algm-AlignAndFocusPowder>` is the main algorithm for correctly focussing diffraction data.
- {ref}`LeBailFit <algm-LeBailFit>` and associated algorithms which can be used to determine an initial FULLPROF parameter file.
- {ref}`ResampleX <algm-ResampleX>` algorithm which allows for specifying the number of bins, and calculates the binning parameters for rebinning. This has been incorporated into {ref}`AlignAndFocusPowder <algm-AlignAndFocusPowder>` and SNSPowderDiffraction

## Single Crystal Diffraction

The focus of the Mantid VATES project (Visualisation and Analysis Toolkit ExtenstionS) has also been single crystal diffraction. This has included additional visualisation tools such as the PeaksViewer, and some improvements to existing tools such as the VatesSimpleInterface. For more details see the specific {ref}`VATES release notes <v2.4.0-vates>`

- Integration techniques have been modified and new ones developed:
  - {ref}`IntegrateEllipsoids <algm-IntegrateEllipsoids>` determines an ellipsoid to integrate around a peak and calculates the background subtracted intensity
  - IntegratePeakTimeSlices has been modified to integrate peaks closer to the edge of the detector
- Access to goniometer angles from python

```
wksp.getRun().getGoniometer().getEulerAngles("yzy")
```

- New user property for the precision of hkl in the table view of a peak workspace. This defaults to two digits past the decimal.
- {ref}`SaveIsawQvector <algm-SaveIsawQvector>` saves a binary file consisting of the Qx, Qy, Qz (in laboratory coordinates) for every event in the input workspace
- {ref}`SCDCalibratePanels <algm-SCDCalibratePanels>` improvements to allow for moving the effective sample position
- Remove rectangular detectors that are not in the NeXus file during load (TOPAZ)
- CreateFractionalPeaks to support magnetic scattering

## Important Fixes

- Goniometer angles read from ISAW peaks file. This only works for files with a single run.
- Error in finding POWGEN runs has been fixed.
