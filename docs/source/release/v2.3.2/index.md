(v2.3.2)=

# Mantid 2.3.2 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

| Patch Release         |                                                                           |
| --------------------- | ------------------------------------------------------------------------- |
| Version               | 2.3.2                                                                     |
| Requested by          | Peter Peterson                                                            |
| Approved by           | Nick Draper, Peter Peterson                                               |
| Authorised on         | 07 Dec 2012                                                               |
| Code Reviewed by      | Michael Reuter, Stuart Campbell, Martyn Gigg, Owen Arnold, Peter Peterson |
| Code Reviewed on      | 10-13 Dec 2012                                                            |
| Review Recommendation | Release                                                                   |
| Released on           | 13 Dec 2012                                                               |

## Key Features

This is a patch release to correct some significant issues since the {ref}`last stable patch release <v2.3.1>`. There are updates to address issues with ADARA related live data, new detectors at SNS, ISIS Indirect analysis, user convenience for Instrument View and segmentation faults/crashes. The full list of changes is below.

## Changes in this version

| Ticket                                                                   | Problem                                                                        | Impact                                                                    | Solution                                                                  |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------------ | ------------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| [#6969](https://github.com/mantidproject/mantid/issues/6969) (Trac 6123) | Enable ICAT4 archive search                                                    | New ADARA files are only cataloged in ICAT4                               | Make old and new SNS and ISIS archive search classes handle new case      |
| [#6984](https://github.com/mantidproject/mantid/issues/6984) (Trac 6138) | Inst view: Allow default view to be specified in inst defn                     | Users have to constantly switch to most used view                         | Add default tag to IDF and have Instrument and InstrumentWindow handle it |
| [#6996](https://github.com/mantidproject/mantid/issues/6996) (Trac 6150) | Create an algorithm for viewing rocking curves/alignment scans                 | Getting total counts versus log value is currently a complicated exercise | Create new algorithm to handle request                                    |
| [#7007](https://github.com/mantidproject/mantid/issues/7007) (Trac 6161) | Disable OpenGL in mantid properties                                            | OpenGL is causing crashes or bad behavior when used via SSH               | Add option to switch off OpenGL for instrument view                       |
| [#7080](https://github.com/mantidproject/mantid/issues/7080) (Trac 6234) | Error in LoadNexusMonitors for events                                          | All events occur in a single TOF in a monitor                             | Fix loop index error                                                      |
| [#7081](https://github.com/mantidproject/mantid/issues/7081) (Trac 6235) | Support Annotation Packets From Live Stream                                    | All live data to handle DAS states properly                               | Update packet definition and parser                                       |
| [#7086](https://github.com/mantidproject/mantid/issues/7086) (Trac 6240) | DGS Reduction doesn't work with new SNS event nexus filenames                  | New ADARA created NeXus files do not reduce                               | Add new extension to file check                                           |
| [#7087](https://github.com/mantidproject/mantid/issues/7087) (Trac 6241) | New POWGEN detector                                                            | There is a new detector that isn't in the geometry                        | Add new detector to POWGEN IDF                                            |
| [#7088](https://github.com/mantidproject/mantid/issues/7088) (Trac 6242) | Add default instrument view to HYSPEC instrument definitions                   | Users have to switch manually to most used view                           | Add default view to IDF                                                   |
| [#7090](https://github.com/mantidproject/mantid/issues/7090) (Trac 6244) | Error in GetDetOffsetsMultiPeaks                                               | There is a segfault that happens for some files                           | Create function to better handle deleting peaks                           |
| [#7105](https://github.com/mantidproject/mantid/issues/7105) (Trac 6259) | Fix run_start value for live listener                                          | Reducing live data fails when run_start is not set                        | Update live listener to set a fake start time until real one available    |
| [#7116](https://github.com/mantidproject/mantid/issues/7116) (Trac 6270) | The run start time needs to have a precision of better than the nearest second | Currently accurate time control is not possible                           | Allow for finer grained control                                           |
| [#7119](https://github.com/mantidproject/mantid/issues/7119) (Trac 6273) | IDF for MANDI                                                                  | A new instrument at SNS is coming online                                  | Add new IDF to Mantid                                                     |
| [#7128](https://github.com/mantidproject/mantid/issues/7128) (Trac 6282) | Add/replace ISIS Indirect fortran modules                                      | Current modules in use at ISIS are broken                                 | Replace module and fix IDA interface to work with new modules             |
| [#7135](https://github.com/mantidproject/mantid/issues/7135) (Trac 6289) | Add default instrument view to ISIS instrument definitions                     | Users have to constantly switch to most used view                         | Add default view to several instruments                                   |
| [#7136](https://github.com/mantidproject/mantid/issues/7136) (Trac 6290) | Add default instrument view to SNS instrument definitions                      | Users have to constantly switch to most used view                         | Add default view to several instruments                                   |
| [#7138](https://github.com/mantidproject/mantid/issues/7138) (Trac 6292) | Solve huge memory usage of Instrument view for MAPS and other instruments      | Instruments with structured detector ids waste a large amount of memory   | Change the cache data type to a map rather than a vector                  |
| [#7141](https://github.com/mantidproject/mantid/issues/7141) (Trac 6295) | Update MANDI monitor distances                                                 | Monitors are not at their current positions                               | Update distances in the IDF to the correct values                         |
| [#7142](https://github.com/mantidproject/mantid/issues/7142) (Trac 6296) | SaveAsciiTest.h not in release-testing code                                    | Having missing file causes patch release code to not compile              | Remove entry from CMakeLists                                              |
| [#7167](https://github.com/mantidproject/mantid/issues/7167) (Trac 6321) | FileFinder search at SNS defect                                                | SNSPowderReduction no longer finds files                                  | Add unit tests and make them work                                         |

## Code Review

### Review of Specific Changes

| Ticket                                                                   | Problem                                                                        | Implementation Notes                                                                                                              | Probability of side effects |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------- | --------------------------- |
| [#6969](https://github.com/mantidproject/mantid/issues/6969) (Trac 6123) | Enable ICAT4 archive search                                                    | Update ISIS and both SNS archive search classes for new search method                                                             | medium                      |
| [#6984](https://github.com/mantidproject/mantid/issues/6984) (Trac 6138) | Inst view: Allow default view to be specified in inst defn                     | Changes to IDF, Instrument and InstrumentWindow                                                                                   | medium                      |
| [#6996](https://github.com/mantidproject/mantid/issues/6996) (Trac 6150) | Create an algorithm for viewing rocking curves/alignment scans                 | New algorithm created, not in use in any core code                                                                                | low                         |
| [#7007](https://github.com/mantidproject/mantid/issues/7007) (Trac 6161) | Disable OpenGL in mantid properties                                            | Change to Mantid properties, ConfigDialog, InstrumentWindow, InstrumentWindowRenderTab and ProjectionSurface to handle new option | medium                      |
| [#7080](https://github.com/mantidproject/mantid/issues/7080) (Trac 6234) | Error in LoadNexusMonitors for events                                          | Single character change                                                                                                           | low                         |
| [#7081](https://github.com/mantidproject/mantid/issues/7081) (Trac 6235) | Support Annotation Packets From Live Stream                                    | Changes to ADARAPackets, ADARAParser and SNSLiveDataEventListener to accomodate new annotations                                   | low                         |
| [#7086](https://github.com/mantidproject/mantid/issues/7086) (Trac 6240) | DGS Reduction doesn't work with new SNS event nexus filenames                  | Change confined to DgsConvertToEnergyTransfer                                                                                     | low                         |
| [#7087](https://github.com/mantidproject/mantid/issues/7087) (Trac 6241) | New POWGEN detector                                                            | Change isolated to POWGEN IDF                                                                                                     | low                         |
| [#7088](https://github.com/mantidproject/mantid/issues/7088) (Trac 6242) | Add default instrument view to HYSPEC instrument definitions                   | Change isolated to HYSPEC IDF                                                                                                     | low                         |
| [#7090](https://github.com/mantidproject/mantid/issues/7090) (Trac 6244) | Error in GetDetOffsetsMultiPeaks                                               | Change isolated to GetDetOffsetsMultiPeaks                                                                                        | low                         |
| [#7105](https://github.com/mantidproject/mantid/issues/7105) (Trac 6259) | Fix run_start value for live listener                                          | Change isolated to SNSLiveEventDataListener                                                                                       | low                         |
| [#7116](https://github.com/mantidproject/mantid/issues/7116) (Trac 6270) | The run start time needs to have a precision of better than the nearest second | Change confined to DateAndTime class, use boost function call                                                                     | medium                      |
| [#7119](https://github.com/mantidproject/mantid/issues/7119) (Trac 6273) | IDF for MANDI                                                                  | Create new IDF                                                                                                                    | low                         |
| [#7128](https://github.com/mantidproject/mantid/issues/7128) (Trac 6282) | Add/replace ISIS Indirect fortran modules                                      | Changes to 3 workflow algorithms and replaced 7 associated pyd file                                                               | low                         |
| [#7135](https://github.com/mantidproject/mantid/issues/7135) (Trac 6289) | Add default instrument view to ISIS instrument definitions                     | Single line changes to the respective IDF files                                                                                   | low                         |
| [#7136](https://github.com/mantidproject/mantid/issues/7136) (Trac 6290) | Add default instrument view to SNS instrument definitions                      | Changes to just ARCS IDF                                                                                                          | low                         |
| [#7138](https://github.com/mantidproject/mantid/issues/7138) (Trac 6292) | Solve huge memory usage of Instrument view for MAPS and other instruments      | Changes isolated to InstrumentActor                                                                                               | medium                      |
| [#7141](https://github.com/mantidproject/mantid/issues/7141) (Trac 6295) | Update MANDI monitor distances                                                 | Changes confined to MANDI IDF                                                                                                     | low                         |
| [#7142](https://github.com/mantidproject/mantid/issues/7142) (Trac 6296) | SaveAsciiTest.h not in release-testing code                                    | One line change to DataHandling's CMakeLists.txt                                                                                  | low                         |
| [#7167](https://github.com/mantidproject/mantid/issues/7167) (Trac 6321) | FileFinder search at SNS defect                                                | Additional units tests and a code refactor                                                                                        | medium                      |

### Are any changes included in the release that are not accounted for above?

No, only the changes from the above tickets have been merged to the release branch.

### Is the collective effect of the changes acceptably low risk?

Yes.

### Code Reviewer Recommendation

Release the code.
