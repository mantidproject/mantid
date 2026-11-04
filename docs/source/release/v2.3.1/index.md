(v2.3.1)=

# Mantid 2.3.1 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

| Patch Release         |                                             |
| --------------------- | ------------------------------------------- |
| Version               | 2.3.1                                       |
| Requested by          | Peter Peterson                              |
| Approved by           | Nick Draper, Peter Peterson                 |
| Authorised on         | 15 Nov 2012                                 |
| Code Reviewed by      | Peter Peterson, Michael Reuter, Owen Arnold |
| Code Reviewed on      | 19 Nov 2012                                 |
| Review Recommendation | Release                                     |
| Released on           | 21 Nov 2012                                 |

## Key Features

This is a patch release to correct some significant issues since the {ref}`last stable release <v2.3.0>`. Particularly problems with streaming live data with event monitors, loading event files from ENGIN-X and a bug in group handling in {ref}`MaskWorkspaceToCalFile <algm-MaskWorkspaceToCalFile>`. The full list of changes is below.

## Changes in this version

| Ticket                                                                   | Problem                                                     | Impact                                                                                                    | Solution                                                                                 |
| ------------------------------------------------------------------------ | ----------------------------------------------------------- | --------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| [#6787](https://github.com/mantidproject/mantid/issues/6787) (Trac 5941) | Make Load() work for .nxs.h5 filenames.                     | This is a valid file extension, but our loader selection was not allowing it.                             | A small change to the file checking by the Event Nexus Loader                            |
| [#6938](https://github.com/mantidproject/mantid/issues/6938) (Trac 6092) | Cannot plot when single workspace has replaced a group      | Bug in plotting if reusing workspace names                                                                | Ensure the Dock data is updated correctly                                                |
| [#6939](https://github.com/mantidproject/mantid/issues/6939) (Trac 6093) | LoadNexusLogs failed to open NXgroup                        | New ADARA files do not have logs                                                                          | Make opengroup call more generic                                                         |
| [#6957](https://github.com/mantidproject/mantid/issues/6957) (Trac 6111) | Support for event monitors                                  | Adara presents monitors in event mode, Mantid needs to support this for ORNL                              | LoadNexusMonitors has been extended to support event based monitors                      |
| [#6967](https://github.com/mantidproject/mantid/issues/6967) (Trac 6121) | Add mantid to python path in etc/mantid.sh                  | Linux users need to do work to use mantid from python                                                     | Additional line in shell files                                                           |
| [#6968](https://github.com/mantidproject/mantid/issues/6968) (Trac 6122) | Crash when reloading Python Algorithm                       | Mantid crashes when reloading an existing python algorithm, this severely impacts the debug/test workflow | The threading of notifications between the UI and scripting environment have been fixed. |
| [#6970](https://github.com/mantidproject/mantid/issues/6970) (Trac 6124) | MaskWorkspacetoCalFile does not handle grouping             | WISH runs with 4-1 mapping will not work                                                                  | Change to the way the python algorithm uses group detectors                              |
| [#6971](https://github.com/mantidproject/mantid/issues/6971) (Trac 6125) | Engin-X event files crash mantid                            | A new event mode instrument cannot process data                                                           | Changes to allow non-contiguous spectra numbers for an event instrument                  |
| [#7004](https://github.com/mantidproject/mantid/issues/7004) (Trac 6158) | CalibrateRectangularDetector uses wrong instrument for SNAP | All instruments with a particular date range are converted into NOMAD                                     | Make a check for the date and NOMAD                                                      |

## Code Review

### Review of Specific Changes

| Ticket                                                                   | Problem                                                     | Implementation Notes                                                                                  | Probability of side effects |
| ------------------------------------------------------------------------ | ----------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- | --------------------------- |
| [#6787](https://github.com/mantidproject/mantid/issues/6787) (Trac 5941) | Make Load() work for .nxs.h5 filenames.                     | Change confined to LoadEventNexus                                                                     | low                         |
| [#6938](https://github.com/mantidproject/mantid/issues/6938) (Trac 6092) | Cannot plot when single workspace has replaced a group      | The change is to force an update to MantidDock                                                        | low                         |
| [#6939](https://github.com/mantidproject/mantid/issues/6939) (Trac 6093) | LoadNexusLogs failed to open NXgroup                        | Change confined to LoadNexusLogs                                                                      | medium                      |
| [#6957](https://github.com/mantidproject/mantid/issues/6957) (Trac 6111) | Support for event monitors                                  | Change confined to LoadNexusMonitors                                                                  | medium                      |
| [#6967](https://github.com/mantidproject/mantid/issues/6967) (Trac 6121) | Add mantid to python path in etc/mantid.sh                  | Change only affects installed config files on linux                                                   | low                         |
| [#6968](https://github.com/mantidproject/mantid/issues/6968) (Trac 6122) | Crash when reloading Python Algorithm                       | Changes are in python algorithm registration (new and old api). Testing shows it works.               | medium                      |
| [#6970](https://github.com/mantidproject/mantid/issues/6970) (Trac 6124) | MaskWorkspacetoCalFile does not handle grouping             | Changes are in the python algorithm only. Changes are minor, look okay, and have been tested to work. | low                         |
| [#6971](https://github.com/mantidproject/mantid/issues/6971) (Trac 6125) | Engin-X event files crash mantid                            | Changes confined to LoadEventNexus                                                                    | medium                      |
| [#7004](https://github.com/mantidproject/mantid/issues/7004) (Trac 6158) | CalibrateRectangularDetector uses wrong instrument for SNAP | Changes confined to CalibrateRectangularDetectors                                                     | low                         |

### Are any changes included in the release that are not accounted for above?

No, individual changes were merged to the patch release.

### Is the collective effect of the changes acceptably low risk?

Yes

### Code Reviewer Recommendation

Release
