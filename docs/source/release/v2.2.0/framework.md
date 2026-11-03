(v2.2.0-framework)=

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

- Live data viewing and processing now supports the Histogram based instruments at ISIS, event support is being worked on and will be in a later version.
- Use the StartLiveData algorithm to connect to a DAE.
- The old mechanism for this is still available as LoadDAE and Load->From DAE, but these will be replaced by the new mechanism in future releases.
- Preliminary support of the ADARA live streaming protocol is included for the SNS, and will be available for it's launch.

## Log data (slow controls)

- A major change in this version is that Log data is now filtered by period and running status by default, so should only contain values of interest to the workspace you access it from.
- You can still access the unfiltered values should you want to but they are no longer the default.
- FilterEvents has been improved to correctly preserve relevant log values.

## Multiperiod handling

- Normalise by current correctly uses per period current values now.
- MergeRuns now correctly handles pairwise merging of multiperiod workspace groups.

## Loading / Saving

- A preview algorithm LoadILLNexus has been introduced for IN5 data. This will be extended over time to support other instruments.
- The progress reporting for SaveNexusProcessed has been improved.
- SaveMD now correctly saves the algorithm history.
- LoadMD can now read MDHistoWorkspace data.

## Grouping

- Group Detectors now processes event data without converting to Histograms
- Group Detectors will correctly handle the mask in the average mode
- Group Detectors will now work for workspaces without and instrument

## Model Optimisation

- Mantid now support graphs from Table Workspaces as an input data source. You can also convert easily normal QtiPlot tables to Table Workspaces using "Convert To Table Workspace" in the Table menu.
- The functions are now grouped for easier access
- HigherOrderPolynomialBackground has been improved and renamed as simply "Polynomial"

## Instrument definitions

- Several instrument definitions have been updated, including OFFSPEC, CRISP and PEARL.

## System Tests

- We have implemented a large number of automated system tests that run full reduction workflows from almost every instrument on a daily basis. This gives the development team better and rapid feedback of any bugs being introduced, or unexpected changes, and will lead to even greater reliability of Mantid for our users workflows.

## Installation

- Mantid is now supported on Ubuntu 12.04
