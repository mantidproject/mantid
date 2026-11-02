(v2.5.3)=

# Mantid 2.5.3 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

| Patch Release         |                             |
| --------------------- | --------------------------- |
| Version               | 2.5.3                       |
| Requested by          | Peter Peterson              |
| Approved by           | Nick Draper, Peter Peterson |
| Authorised on         | 16 May 2013                 |
| Code Reviewed by      | Owen Arnold                 |
| Code Reviewed on      | 27 May 2013                 |
| Review Recommendation | Release                     |
| Released on           | 31 May 2013                 |

## Key Features

This is a patch release to correct some significant issues since the {ref}`last full release <v2.5.0>`. There are updates to include changes to the POWGEN instrument, improvements to the script repository and a few irritating bugs fixed. The full list of changes is below.

## Changes in this version

| Ticket                                                                   | Problem                                                                   | Impact                                                                                                | Solution                                                                                                            |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| [#7917](https://github.com/mantidproject/mantid/issues/7917) (Trac 7071) | Update POWGEN geometry                                                    | Preparatory work for new detectors being installed during the summer shutdown                         | Current IDF set to expire May 31st at 23:59 new IDF valid for June 1st at 00:01 and ending July 31 at 23:59         |
| [#7569](https://github.com/mantidproject/mantid/issues/7569) (Trac 6723) | Add additional grouping level to POWGEN instrument definitions            | Allow for additional view when performing detector diagnostics                                        | Fixed in the IDF                                                                                                    |
| [#7947](https://github.com/mantidproject/mantid/issues/7947) (Trac 7101) | Crash in PythonAlgorithm using child alg and replacing workspace directly | Trouble updating python algorithms to v2.                                                             | Fix reference counting using Weak Pointers.                                                                         |
| [#7930](https://github.com/mantidproject/mantid/issues/7930) (Trac 7084) | Mantidplot crash when changing the connecting line                        | Changes of line colour or connection style led to crashes                                             | Fix the bug that was introduced while fixing a Mac issue.                                                           |
| [#7943](https://github.com/mantidproject/mantid/issues/7943) (Trac 7097) | Files downloaded from scriptrepo should be immediately usable in scripts  | Python directories needs to be set up to match the script repository location.                        | The script repository directories are set up automatically as python directories.                                   |
| [#7949](https://github.com/mantidproject/mantid/issues/7949) (Trac 7103) | Migrate pythonalgorithms.directories values automatically                 | Users did not know what to do with the deprecation warning for pythonalgorithms.directories           | Migrate the values to the new key automatically.                                                                    |
| [#7948](https://github.com/mantidproject/mantid/issues/7948) (Trac 7102) | Update changes page for python api to include all differences             | Some of the differences in v2 of the Python API were missing.                                         | Complete the documentation [here](http://download.mantidproject.org/docs/current-release/python/html/changes.html). |
| [#7950](https://github.com/mantidproject/mantid/issues/7950) (Trac 7104) | Add a help "?" button to the script repository                            | It was not clear how to find the help for the script repository.                                      | Add a help "?" button.                                                                                              |
| [#7860](https://github.com/mantidproject/mantid/issues/7860) (Trac 7014) | VSI menu disappears on Mac OS X                                           | The menu did not appear on Mac OS X.                                                                  | Fixed the menu's appearance.                                                                                        |
| [#7962](https://github.com/mantidproject/mantid/issues/7962) (Trac 7116) | Update CanSAS1D header                                                    | XSL formatting was not working.                                                                       | Fixed the XSL link for v2 of the algorithm.                                                                         |
| [#7940](https://github.com/mantidproject/mantid/issues/7940) (Trac 7094) | Change default instrument view for MANDI and TOPAZ                        | The default view of 3D is not the most suitable.                                                      | Set the default view to be Spherical Y for both instruments.                                                        |
| [#7958](https://github.com/mantidproject/mantid/issues/7958) (Trac 7112) | SNSPowderReduction fails to find correct vanadium run                     | Processing multiple runs get incorrect characterization runs                                          | Fix run determination                                                                                               |
| [#7996](https://github.com/mantidproject/mantid/issues/7996) (Trac 7150) | SNSPowderReduction fails with splitting to multiple workspaces            | SNSPowderReduction fails with workspaces that use event splitting                                     | correctly handle event splitting                                                                                    |
| [#7985](https://github.com/mantidproject/mantid/issues/7985) (Trac 7139) | SliceViewer Crashes with ellipsoidal intergration                         | Unfortunate crash if this integration method is used                                                  | Add protection around the internal assumption that a parameter exists.                                              |
| [#7998](https://github.com/mantidproject/mantid/issues/7998) (Trac 7152) | ResultsLog print statements from Python end up double-spaced              | The results log is hard to follow.                                                                    | Remove the additional line feed character.                                                                          |
| [#8019](https://github.com/mantidproject/mantid/issues/8019) (Trac 7173) | Memory hog & crash when plotting filtered sample time series logs         | Memory Leak when combining filters, particularly if one is invalid.                                   | A check has been added that the filters are valid.                                                                  |
| [#4240](https://github.com/mantidproject/mantid/issues/4240) (Trac 3393) | ConvertToPointData axis problem                                           | The vertical axis is lost when using ConvertToPointData if it is anything other than a spectrum axis. | Correctly copy over the axis from the input workspace.                                                              |
| [#8005](https://github.com/mantidproject/mantid/issues/8005) (Trac 7159) | SANS need an I(Q) subtraction algorithm                                   | SANS users needed to remove a background distribution from reduced I(Q)                               | New Algorithm added SANSSubtract.                                                                                   |
| [#7944](https://github.com/mantidproject/mantid/issues/7944) (Trac 7098) | Error in SliceView peaks list                                             | In some circumstances incorrect values appeared in the Itens and SigInt columns.                      | Bug resolved.                                                                                                       |
| [#8058](https://github.com/mantidproject/mantid/issues/8058) (Trac 7212) | SliceViewer from VSI MultiSlice view no longer works                      | SliceViewer views launched from the VSI multislice view do not match after a rebinning step.          | Bug resolved.                                                                                                       |

## Code Review

### Review of Specific Changes

| Ticket                                                                   | Problem                                                                   | Implementation Notes                                               | Probability of side effects | Merged |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------- | ------------------------------------------------------------------ | --------------------------- | ------ |
| [#7917](https://github.com/mantidproject/mantid/issues/7917) (Trac 7071) | Update POWGEN geometry                                                    |                                                                    | Low                         | May 21 |
| [#7569](https://github.com/mantidproject/mantid/issues/7569) (Trac 6723) | Add additional grouping level to POWGEN instrument definitions            |                                                                    | Low                         | May 21 |
| [#7947](https://github.com/mantidproject/mantid/issues/7947) (Trac 7101) | Crash in PythonAlgorithm using child alg and replacing workspace directly | Reference counting issue fixed using weak pointers                 | Med                         | May 28 |
| [#7930](https://github.com/mantidproject/mantid/issues/7930) (Trac 7084) | Mantidplot crash when changing the connecting line                        |                                                                    | Low                         | May 29 |
| [#7943](https://github.com/mantidproject/mantid/issues/7943) (Trac 7097) | Files downloaded from scriptrepo should be immediately usable in scripts  | Multiple small changes. Sensible changes.                          | Low                         | May 29 |
| [#7949](https://github.com/mantidproject/mantid/issues/7949) (Trac 7103) | Migrate pythonalgorithms.directories values automatically                 | Auto migrate/add location. Configurable via properties menu.       | Med                         | May 28 |
| [#7948](https://github.com/mantidproject/mantid/issues/7948) (Trac 7102) | Update changes page for python api to include all differences             | Update documentation                                               | Low                         | May 29 |
| [#7950](https://github.com/mantidproject/mantid/issues/7950) (Trac 7104) | Add a help "?" button to the script repository                            | Simple add button                                                  | Low                         | May 28 |
| [#7860](https://github.com/mantidproject/mantid/issues/7860) (Trac 7014) | VSI menu disappears on Mac OS X                                           | Set parent widget                                                  | Low                         | May 28 |
| [#7962](https://github.com/mantidproject/mantid/issues/7962) (Trac 7116) | Update CanSAS1D header                                                    | Customise the header                                               | Low                         | May 29 |
| [#7940](https://github.com/mantidproject/mantid/issues/7940) (Trac 7094) | Change default instrument view for MANDI and TOPAZ                        | Simple change to the Instrument definition to set the default view | Very Low                    | May 29 |
| [#7958](https://github.com/mantidproject/mantid/issues/7958) (Trac 7112) | SNSPowderReduction fails to find correct vanadium run                     |                                                                    | Low                         | May 21 |
| [#7996](https://github.com/mantidproject/mantid/issues/7996) (Trac 7150) | SNSPowderReduction fails with splitting to multiple workspaces            |                                                                    | Low                         | May 23 |
| [#7985](https://github.com/mantidproject/mantid/issues/7985) (Trac 7139) | SliceViewer Crashes with ellipsoidal intergration                         | Provide a radius for the integrated workspace                      | Low                         | May 29 |
| [#7998](https://github.com/mantidproject/mantid/issues/7998) (Trac 7152) | ResultsLog print statements from Python end up double-spaced              | Add new line as signal emitted                                     | Low                         | May 29 |
| [#8019](https://github.com/mantidproject/mantid/issues/8019) (Trac 7173) | Memory hog & crash when plotting filtered sample time series logs         | Calculate size fix to avoid errors in size calculations            | Low                         | May 29 |
| [#4240](https://github.com/mantidproject/mantid/issues/4240) (Trac 3393) | ConvertToPointData axis problem                                           | Copy over the vertical axis if required                            | Low                         | May 29 |
| [#8005](https://github.com/mantidproject/mantid/issues/8005) (Trac 7159) | SANS I(Q) subtraction algorithm                                           | New algorithm                                                      | Low                         | May 30 |
| [#7944](https://github.com/mantidproject/mantid/issues/7944) (Trac 7098) | Error in slice view peaks list                                            | Fix simple copy paste error                                        | Low                         | May 30 |
| [#8058](https://github.com/mantidproject/mantid/issues/8058) (Trac 7212) | SliceViewer from VSI MultiSlice view no longer works                      | Update axis prior to export                                        | Low                         | May 30 |

All changes are on the [release/2.5.x](http://github.com/mantidproject/mantid/tree/release/2.5.x) branch.

### Are any changes included in the release that are not accounted for above?

No changes have been introduced that are not directly related to the issues listed above. Either the feature branches have been generated from the v2.5 tag and merged in, or they have been cherry-picked onto the release branch.

### Is the collective effect of the changes acceptably low risk?

Most changes are low risk. All changes have been manually re-tested. Those changes that have a medium risk, are well covered by unit and system tests. Most likely, these changes would cause the test suite to fail if they had introduced side effects.

### Code Reviewer Recommendation

Release
