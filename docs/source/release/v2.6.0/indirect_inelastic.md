(v2.6.0-indirect_inelastic)=

# Indirect Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

## ElWin

- Has been updated so that the name of the sample environment parameter, temperature for example, can be changed from its default value of 'sample'. There is a new option to Normalise to Lowest Temperature, if selected the \*\_eq1 workspace is normalised to the intensity of the lowest Q value of the lowest temperature run. This will allow better comparisons between different samples and instruments.

```{figure} ../../images/2_6_release/FuryFitWithError.webp
---
class: screenshot
width: 500px
align: right
---
FuryFit with error bars
```

### MSDFit, FuryFit, ConvFit and Qlines

- The fitting routines msdFit, FuryFit, ConvFit and Qlines have a new workspace named \*\_Result. This is a group workspace with a workspace for each histogram/spectrum containing 3 histograms labelled Data, Calc & Diff where Diff is the difference between the calculated fit (Calc) and the input (Data). This enables easy checking of the individual spectrum fits.
- FuryFit and ConvFit have been modified to now include error bars.

### Apply Corrections

- Has a \*\_Result workspace with the individual workspaces having 3 histograms labelled Sample, Can & Calc. Sample & Can are the input data and Calc is the final absorption corrected spectrum. This enables checking as to how the corrections have performed.

### Indirect Diffraction

- Has a new tab for OSIRIS which is similar to IRIS with 2 options of analysis : diffonly & diffspec. Diffonly should be used for powder diffraction analysis while diffspec is used during spectroscopy runs and just shows the d-spacing for all detectors combined. There is also an option to rebin the d-spacing.

### IDA and C2E Interface

- Both the IDA and C2E interface have been updated to automatically refresh when new workspaces are added or created. This has led to the removal of the refresh button from the C2E SofQ tab.

```{figure} ../../images/2_6_release/C2EReferesh2.5.webp
---
class: screenshot
width: 100px
---
C2E interface 2.5 with refresh button removed in 2.6
```

```{figure} ../../images/2_6_release/C2ERefresh2.6.webp
---
class: screenshot
width: 101px
---
C2E interface in 2.6, with automatically refreshing workspace selectors
```

## Noteworthy Fixes

- Fixed an error in loading BASIS data where an anomalously high number of events were assigned to the first pixel ([#8369](https://github.com/mantidproject/mantid/issues/8369) (Trac 7524)).
