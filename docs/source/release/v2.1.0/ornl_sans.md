(v2.1.0-ornl_sans)=

# ORNL SANS

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_1_release/Patch.webp
---
class: screenshot
width: 400px
align: right
---
Patched EQSANS detector sensitivity (right). The detector image on the left shows the areas that were patched.
```

## Algorithm Improvements

1. Added functionality to produce a patched sensitivity file. User can define a region of the detector they want patched. After masking that region and computing the detector sensitivity, the {ref}`EQSANSPatchSensitivity <algm-EQSANSPatchSensitivity>` algorithm will assign each patched pixel to the average of all unmasked and unpatched pixels along that pixel's tube.

1. Added the option to select the initial wavelength binning of EQSANS event data {ref}`EQSANSLoad <algm-EQSANSLoad>`.

1. Added the option to specify a specific beam flux file rather than using the default beam flux distribution {ref}`EQSANSNormalise <algm-EQSANSNormalise>`.
