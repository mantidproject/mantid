(v2.3.0-framework)=

# Framework

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

```{figure} ../../images/2_2_release/LiveDataMonitoring.webp
---
class: screenshot
width: 400px
align: right
alt: LiveDataMonitoring
---
```

## Live Data Processing

- The live listener now fully supports the data from the ADARA project at the SNS, so ADARA supported instruments can make full use of the live data processing within Mantid

## Loading / Saving

- Mantid now supports changes in the formatting of the names of data files over time. This has been put in place initially for INTER, SURF, IRIS and OSIRIS, but is able to support any instrument. Additional file name structures simply need to be added to the facilities.xml file (or send an email to mantid-help@mantidproject.org for help).
- Protection has been added around multiple file loading and summing to prevent input errors leading to Mantid trying to load thousands of data files. If you run into the current limit the message explains how to increase the number of files allowed.

## Group Workspaces

- Several improvements have been made to group workspaces and the workspace display in Mantidplot to resolve bugs that would occasionally make it appear that workspaces had escaped from their group or other strange behaviour.
- Mantid can now determine if a group workspace is the result of multiperiod data, or the result of ad-hoc grouping. This can also be determined via python.

## Model Optimisation

```{figure} ../../images/2_3_release/Voigt.webp
---
class: screenshot
width: 400px
align: right
alt: Voigt
---
```

- A new Voigt peak function has been added.
- The Back to Back Exponential function has been improved
- Damping Optimizer has been added, which is a version of Levenberg-Marquardt with constant damping.
- Calculated derivatives now take tied parameters into account.
- A new option has been added to the Fit button of the Fitting toolbox. "Evaluate" will output a workspace of the current functions without optimization.
- Instrument fit parameters now correctly survive saving and reloading from Nexus files.

## Installation

- SciPy, MatplotLib and iPython have all been bundled into the Mantid installer for Windows and Ubuntu in this release, and are therefore all available "out of the box".
- The Mac installer has been slimmed down to take less time to download.
