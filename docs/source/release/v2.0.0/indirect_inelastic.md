(v2.0.0-indirect_inelastic)=

# Indirect Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## General

- Calls to `LoadRaw` replaced with calls to `Load`.
- **Rebinning monitors** now available in the reduction by adding a property to the instrument parameter file.
- Initial support for the BASIS instrument at the SNS.
- New algorithm `ModeratorTzero` to correct for moderator emission time uncertainty. This is controlled by properties in the instrument parameter file.

## Indirect Data Analysis Interface

- Modifications/corrections to **MSDFit** and **AbsCorr**, including:

  - replacing the use of the deprecated `Linear` algorithm with `Fit`;
  - improved plotting; and
  - improved workspace naming.

- **ConvFit** improvement, so that more than the default range can be used.

## Convert to Energy Interface

- **S(Q,w)** changes:

  - The ability to choose between `SofQW` algorithm which uses centre point rebinning, and the newer `SofQW2` which rebins using overlapping polyhedra.
  - Slight UI improvements.

- **Vibrational Spectroscopy** (i.e. TOSCA) -specific changes to to the reduction:

  - A third, "average", spectra is generated when loading a run.
  - Support for saving to "Aclimax" format, and to output in cm-1.

```{figure} ../../images/2_0_release/Release_Notes_2_0_CTE_UI_Changes.webp
---
class: screenshot
width: 800px
align: right
alt: Release Notes 2 0 CTE UI Changes
---
```
