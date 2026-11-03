(v2.3.0-reflectometry)=

# Reflectometry

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## New Features

- Shipped ISIS Reflectometry Stitching GUI
- Make algorithms multi-period group aware
- First INTER, SURF instrument definition files to be shipped with Mantid.
- CRISP instrument definition file updated
- SNS Liquids Reflectometry Interface changes
  - Slightly new look of the SNS Liquids Reflectometry Interface
    - Added metadata information
    - Changed some of the default output parameters (labels, Qmin and Qwidth)\]\]
  - Add prototype version of 2D ROI selection tool.

## Defects Fixed

- Fix Stitch1DMD to work properly with multi-period groups
- {ref}`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` should copy experiment info to output workspaces
