(v2.4.0-direct_inelastic)=

# Direct Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_4_release/MultiRotation_Split_Zoom_Sliceviewer.webp
---
class: screenshot
width: 400px
align: right
alt: MultiRotation Split Zoom Sliceviewer
---
```

## Updates

- Ability to split/process a multi-rotation experiment into a combined multi-dimensional data set which can then be easily visualised (see figure).
- The formula used to calculate the moderator T{sub}`0` for HYSPEC has been updated to be {math}`T_0 = 4+(107/(1+(E_i/31)^3))`.
- It is now possible to load the simulated neutron event output from McVine/VNF. These are loaded either as a Mantid Workspace NeXus file or as a Raw Event NeXus file.
- ARCS and HYSPEC have a default unwrap view defined for the Instrument View.

## Issues Fixed

- {ref}`DgsReduction <algm-DgsReduction>` now works with new 'ADARA' NeXus files (currently only produced on HYSPEC at the SNS)
- Minor issue with {ref}`GenerateGroupingSNSInelastic <algm-GenerateGroupingSNSInelastic>` not including instrument name.
