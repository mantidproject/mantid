(v2.1.0-reflectometry)=

# Reflectometry

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_1_release/Refm_ui.webp
---
class: screenshot
width: 400px
align: right
---
Screenshot of the new REFM Reduction UI.
```

## Algorithm Improvements

1. Fixed {ref}`ConvertUnits <algm-ConvertUnits>` to use signed two theta for instruments where use of signed two theta has been enabled.

## System Testing

1. Basic ISIS relectometry workflow is now part of our integrated system testing and benchmarking.

## Magnetic Reflectometry at SNS

1. Initial support for specular reflectivity measurements at the SNS magnetic reflectometer has been added to Mantid. The RefReduction algorithm can reduce reflectivity data for both magnetic and non-magnetic measurements.
1. The REF_M Reduction user interface has been added. It allows users to reduce their reflectivity data and stitch several reflectivity curves into a single measurement.
