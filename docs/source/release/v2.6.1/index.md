(v2.6.1)=

# Mantid 2.6.1 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

| Patch Release         |                 |
| --------------------- | --------------- |
| Version               | 2.6.1           |
| Requested by          | Nick Draper     |
| Approved by           | Stuart Campbell |
| Authorised on         | 08 Aug 2013     |
| Code Reviewed by      | Stuart Campbell |
| Code Reviewed on      | 12-15 Aug 2013  |
| Review Recommendation | Release         |
| Released on           | 20 Aug 2013     |

## Key Features

This is a patch release to correct some significant issues since the {ref}`last full release <v2.6.0>`. There are updates to fix a threading issue causing problems with the reduction of certain large instruments, a problem when adding SANS event files, improvements to MPI processing, corrections to some instrument geometries and some improvements to EQSANS reduction.

## Changes in this version

| Ticket                                                                   | Problem                                                                                  | Impact                                                                                 | Solution                                                                                                     |
| ------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| [#8398](https://github.com/mantidproject/mantid/issues/8398) (Trac 7553) | Improvements to NOMAD autoreduction with MPI                                             | Problems with the MPI reduction prevented usage.                                       | Rebin into histograms rather than maintain event data. ensure correct chunking of the can and vanadium files |
| [#8445](https://github.com/mantidproject/mantid/issues/8445) (Trac 7600) | Defect on SANS2D adding data                                                             | Event data files can be corupted when adding files togeter using the SANS interface.   | Correctly take into account any monitors when merging event files                                            |
| [#8522](https://github.com/mantidproject/mantid/issues/8522) (Trac 7677) | Reduction for several inelastic instruments hangs part way through reduction             | Instruments such as TOSCA, MERLIN and LET cannot use Mantid v2.6 to reduce their data. | Use lazy loading for Workspace details and improve locking approaches                                        |
| [#8530](https://github.com/mantidproject/mantid/issues/8530) (Trac 7685) | MPI runs can fail in SNSPowderReduction                                                  | In sertain situations the reduction could fail part way through                        | Added a check that the data is indeed still in events before running CompressEvents.                         |
| [#8544](https://github.com/mantidproject/mantid/issues/8544) (Trac 7699) | EQSANS: reducing existing workspace fails                                                | Reduction has to use the file every time even if it is already loaded                  | This used to work in previous versions, correct the area of the scripts that does not support this           |
| [#8546](https://github.com/mantidproject/mantid/issues/8546) (Trac 7701) | EQSANS: make sure transmission workspaces are created                                    |                                                                                        |                                                                                                              |
| [#8550](https://github.com/mantidproject/mantid/issues/8550) (Trac 7705) | Load not working with Dave grouped file with different extension                         | GRP files with a .dat extension are not shown                                          | Add \*.dat onto the list of extensions                                                                       |
| [#8563](https://github.com/mantidproject/mantid/issues/8563) (Trac 7718) | "Do Not Rebin" option causes error with TOSCA                                            |                                                                                        |                                                                                                              |
| [#8565](https://github.com/mantidproject/mantid/issues/8565) (Trac 7720) | Problem with boolean properties and GUI                                                  | Boolean properties that have a default of true do not get set                          | Revert change that caused the issue                                                                          |
| [#8589](https://github.com/mantidproject/mantid/issues/8589) (Trac 7744) | Default values are not being shown when running algorithm dialog form script interpreter | Some algorithms might not work in the way that users are used to                       | Fix the problem with default value logic                                                                     |
| [#8611](https://github.com/mantidproject/mantid/issues/8611) (Trac 7766) | Mistake in latest powgen geometry                                                        | POWGEN files from current run cycle                                                    | Updated instrument geometry                                                                                  |

## Code Review

### Review of Specific Changes

| Ticket                                                                   | Problem                                                                                  | Implementation Notes           | Probability of side effects | Merged |
| ------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------- | ------------------------------ | --------------------------- | ------ |
| [#8398](https://github.com/mantidproject/mantid/issues/8398) (Trac 7553) | Improvements to NOMAD autoreduction with MPI                                             |                                | Medium                      | 12-Aug |
| [#8445](https://github.com/mantidproject/mantid/issues/8445) (Trac 7600) | Defect on SANS2D adding data                                                             | Only effects ISIS SANS         | Low                         | 12-Aug |
| [#8522](https://github.com/mantidproject/mantid/issues/8522) (Trac 7677) | Reduction for several inelastic instruments hangs part way through reduction             |                                | Medium                      | 12-Aug |
| [#8530](https://github.com/mantidproject/mantid/issues/8530) (Trac 7685) | MPI runs can fail in SNSPowderReduction                                                  |                                | Low                         | 12-Aug |
| [#8544](https://github.com/mantidproject/mantid/issues/8544) (Trac 7699) | EQSANS: reducing existing workspace fails.                                               |                                | Low                         | 12-Aug |
| [#8546](https://github.com/mantidproject/mantid/issues/8546) (Trac 7701) | EQSANS: make sure transmission workspaces are created                                    |                                | Medium                      | 13-Aug |
| [#8550](https://github.com/mantidproject/mantid/issues/8550) (Trac 7705) | Load not working with Dave grouped file with different extension                         | \*.dat added                   | Low                         | 15-Aug |
| [#8563](https://github.com/mantidproject/mantid/issues/8563) (Trac 7718) | "Do Not Rebin" option causes error with TOSCA                                            |                                | Low                         | 13-Aug |
| [#8565](https://github.com/mantidproject/mantid/issues/8565) (Trac 7720) | Problem with boolean properties and GUI                                                  | Only effects EQSANS            | Low                         | 13-Aug |
| [#8589](https://github.com/mantidproject/mantid/issues/8589) (Trac 7744) | Default values are not being shown when running algorithm dialog form script interpreter |                                | Low                         | 19-Aug |
| [#8611](https://github.com/mantidproject/mantid/issues/8611) (Trac 7766) | Mistake in latest powgen geometry                                                        | Only effect newer POWGEN files | Low                         | 16-Aug |

### Are any changes included in the release that are not accounted for above?

None

### Is the collective effect of the changes acceptably low risk?

Yes

### Code Reviewer Recommendation

Deploy
