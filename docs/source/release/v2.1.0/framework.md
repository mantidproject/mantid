(v2.1.0-framework)=

# Framework

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Calibration of detectors

Additional information has been added to the saved nexus files that mantid creates to allow them to be used to pass on details of calibrated detector positions to data acquisition to be included in subsequent run files. A first attempt at the calibration script for the PSD based instruments LET and MERLIN has been included in this release, and will evolve rapidly. We are working with Data acquisition at ISIS to allow nexus files to be provided instead of a .DAT file to provide the calibration for subsequent runs.

## Automated data reduction

Mantid is now available as a slimmed down package without the graphical user interface for use in automated data reduction where user input is not required. This is currently in use on POWGEN, NOMAD, SEQUOIA and BASIS at the SNS.

## Archive Search for the SNS

The archive search has been rewritten at the SNS to fully support their indexing systems, so now runs can be found automatically in Mantid by just providing the run number or instrument + run number. As it is no longer required the FindSNSNeXus algorithm has been removed.

## Improved threading for Event Workspaces

Event workspaces have been improved so that they can be use in multithreaded operations much more frequently. The underlying issue is that the workspace is not always threadsafe, but it now tracks it's suitability for multithreaded operations, and will convert itself to a threadsafe state if required. This has improved the performance of a wide range of operations with event workspaces.

## SofQW & Rebin 2D

Have been improved to correctly account for the normalisation required when accounting for the fractional area created when comparing the two grids involved in the rebin. It also correctly accounts for non observable areas or masked when calculating the contributing pixels. More details are in the {ref}`Direct Inelastic <v2.1.0-direct_inelastic>` release notes.

## Mask Workspaces

Mask workspaces are a workspace that purely describes the masking pattern for a set of data. These were introduced in the previous release, but their use and funcationality has been significantly increased in this release.

- Instrument View Masking now saves masks using SaveMask
- ExtrackMask will extract a MaskWorkspace from any other workspace
- LoadMask will load a MaskWorkspace from a variety of file formats
- InvertMask will ... invert a mask

## Algorithm name changes

In a continued attempt to keep the naming of our algorithms sensible we have renamed a few algorithms so they better fit our naming test "Does it do what it says (on the tin)". The old names will continue to work for this release to ensure backward comparability.

- ConjoinWorkspaces2 -> AppendSpectra
- ConvertToMaskingWorkspace -> ConvertToMaskWorkspace
- SetUncertaintiesToZero -> SetUncertainties
- ExtractMasking -> InvertMask
- SaveDetectorMask -> SaveMask
- LoadMaskingFile -> LoadMask
- getDimensionNamed -> getDimensionWithId

## Multiple file loading and summing

All load algorithms now support mutliple file loading and summing. So now you can enter several file to be either loaded seperately, or summed into one with a single call to the algorithm. Some examples of the syntax used are:

```
MAR17198 + 17199 + 17240
 Will load and sum the three files
MAR17198, 17199, 17240
 Will load the three files separately
MAR17198-17240
 Will load and sum all of the files between 17198 and 17240 inclusively
MAR17198:17240
 Will load all of the files between 17198 and 17240 inclusively as separate workspaces
MAR17198-17240:2
 Will load and sum all of the files between 17198 and 17240 inclusively, with a step of 2, therefore missing out every other file
MAR17198:17240:2
 Will load all of the files between 17198 and 17240 inclusively as separate workspaces, with a step of 2, therefore missing out every other file
```

## Model Optimisation

The model optimisation and curve fitting engine has bee refactored to all efficient operations on very large datasets, such as those found in multi dimensional workspaces. Several new fit functions, including flat background, stretched exponential and several muon specific functions.

## Instrument definitions

Instrument definitions now support overlapping validity date ranges, if two files are valid the latest is selected.

## Mantid projects

Waterfall plots and curve fit functions are now included in the saving/loading of MantidPlot projects
