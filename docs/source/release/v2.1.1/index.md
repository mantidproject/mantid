(v2.1.1)=

# Mantid 2.1.1 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

| Patch Release         |                                              |
| --------------------- | -------------------------------------------- |
| Version               | 2.1.1                                        |
| Requested by          | Nick Draper                                  |
| Approved by           | Nick Draper, Stuart Campbell, Peter Peterson |
| Authorised on         | 24 May 2012                                  |
| Code Reviewed by      | Owen Arnold                                  |
| Code Reviewed on      | 30 May 2012                                  |
| Review Recommendation | Release                                      |
| Released on           | 1 June 2012                                  |

## Key Features

This is a patch release to correct some significant issues since the {ref}`last stable release <v2.1.0>`.

## Changes in this version

| Ticket                                                                   | Problem                                                                         | Impact                                                                         | Solution                                                                |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------------- | ------------------------------------------------------------------------------ | ----------------------------------------------------------------------- |
| [#6134](https://github.com/mantidproject/mantid/issues/6134) (Trac 5288) | Clicking Apply/OK in the user preferences dialog throws an exception on Windows | Usability is badly affected                                                    | Improved safety of path string conversion.                              |
| [#6180](https://github.com/mantidproject/mantid/issues/6180) (Trac 5334) | ConjionWorkspaces parameter list has changed                                    | Broken user scripts                                                            | Update ConjoinFiles and ConjoinSpectra to match                         |
| [#6181](https://github.com/mantidproject/mantid/issues/6181) (Trac 5335) | Uninitialized variable in SNSPowderReduction summing option                     | SNSPowder reduction script can fail.                                           | Add missing initialization                                              |
| [#6192](https://github.com/mantidproject/mantid/issues/6192) (Trac 5346) | NOMAD low angle bank definition appears to be wrong.                            | Results do not match IDL code.                                                 | Verify it is correct or update instrument definition.                   |
| [#6197](https://github.com/mantidproject/mantid/issues/6197) (Trac 5351) | SNSPowderReduction broken for mixed wavelength runs                             | Data cannot be processed                                                       | Shorten the caching duration of the info object.                        |
| [#6199](https://github.com/mantidproject/mantid/issues/6199) (Trac 5353) | LoadEventNexus with ISIS data & MonitorsAsEvents fails                          | LET data cannot be processed                                                   | Relax optimisation & correct spectrum number mapping                    |
| [#6233](https://github.com/mantidproject/mantid/issues/6233) (Trac 5387) | Display tab, for Guinier plots etc, crashes Mantid                              | Functionality is unusable                                                      | Correct the order of fitting parameters                                 |
| [#6207](https://github.com/mantidproject/mantid/issues/6207) (Trac 5361) | MERLIN tubes need unique names                                                  | Hard to differentiate tubes, particularly on calibration                       | Correct minimizers stop condition for linear fits.                      |
| [#6210](https://github.com/mantidproject/mantid/issues/6210) (Trac 5364) | Muon: bug calculating dead time                                                 | Unnecessary warning messages are produced                                      | Add T0 equation in python script.                                       |
| [#6237](https://github.com/mantidproject/mantid/issues/6237) (Trac 5391) | Invert does not work on MaskWorkspaceToCalFile                                  | Prevents WISH cal file creation                                                | provide implementation for property.                                    |
| [#6239](https://github.com/mantidproject/mantid/issues/6239) (Trac 5393) | SaveMask fails with WISH data                                                   | Unable to create wish cal file                                                 | Improve handling of grouped detectors.                                  |
| [#6240](https://github.com/mantidproject/mantid/issues/6240) (Trac 5394) | Instrument View: Save mask does not apply the mask first.                       | Empty files are produced, users are confused.                                  | Apply the mask before saving.                                           |
| [#6242](https://github.com/mantidproject/mantid/issues/6242) (Trac 5396) | Instrument View: Improve the text under the save button.                        | Both the new and old text options were confusing.                              | More explanatory text options have been added.                          |
| [#5682](https://github.com/mantidproject/mantid/issues/5682) (Trac 4835) | Improve the integration of peaks using IkedaCarpeter fits of TOF data.          | Background was not being included.                                             | Change the column of the table used as the data source.                 |
| [#6051](https://github.com/mantidproject/mantid/issues/6051) (Trac 5205) | Exceptions in Load with mixed file types.                                       | Confusion when loading files when multiple types of the same file are present. | ??.                                                                     |
| [#6145](https://github.com/mantidproject/mantid/issues/6145) (Trac 5299) | Instruments with detectrs missing from the definition cannot be displayed       | GEM cannot be displayed.                                                       | Ignore spectra with detectors not present in the instrument definition. |
| [#6147](https://github.com/mantidproject/mantid/issues/6147) (Trac 5301) | Update needed to offspec definition.                                            | Manual updates needed otherwise.                                               | Include updated file.                                                   |
| [#6156](https://github.com/mantidproject/mantid/issues/6156) (Trac 5310) | Python API switches leave some commands unchanged.                              | Obscure bugs.                                                                  | Ensure python switch is complete.                                       |
| [#6241](https://github.com/mantidproject/mantid/issues/6241) (Trac 5395) | Run object usage is different in the new API                                    | scripts do not update to new API                                               | Support Dict stlye access.                                              |
| [#6002](https://github.com/mantidproject/mantid/issues/6002) (Trac 5156) | Update to offspec scripts.                                                      | Otherwise manual updates required.                                             | Updated file included.                                                  |

## Code Review

### Review of Specific Changes

| Ticket                                                                   | Problem                                                                         | Implementation Notes                                                                                           | Probability of side effects |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- | --------------------------- |
| [#6134](https://github.com/mantidproject/mantid/issues/6134) (Trac 5288) | Clicking Apply/OK in the user preferences dialog throws an exception on Windows | Changes confined to the config service as expected.                                                            | Low.                        |
| [#6180](https://github.com/mantidproject/mantid/issues/6180) (Trac 5334) | ConjoinWorkspaces parameter list has changed                                    | Removal of parameter in the python call.                                                                       | Low.                        |
| [#6181](https://github.com/mantidproject/mantid/issues/6181) (Trac 5335) | Uninitialized variable in SNSPowderReduction summing option                     | Variable gets initialized.                                                                                     | Low.                        |
| [#6192](https://github.com/mantidproject/mantid/issues/6192) (Trac 5346) | NOMAD low angle bank definition appears to be wrong.                            | Modifications to xml files. Should not break core functionality.                                               | Low.                        |
| [#6197](https://github.com/mantidproject/mantid/issues/6197) (Trac 5351) | SNSPowderReduction broken for mixed wavelength runs                             | Simple modification to python script                                                                           | Low.                        |
| [#6199](https://github.com/mantidproject/mantid/issues/6199) (Trac 5353) | LoadEventNexus with ISIS data & MonitorsAsEvents fails                          | Changes seem sensible. Backed up by system tests.                                                              | Medium.                     |
| [#6233](https://github.com/mantidproject/mantid/issues/6233) (Trac 5387) | Display tab, for Guinier plots etc, crashes Mantid                              | Low risk changes affecting to expected code sections.                                                          | Low.                        |
| [#6207](https://github.com/mantidproject/mantid/issues/6207) (Trac 5361) | MERLIN tubes need unique names                                                  | Update to instrument xml files                                                                                 | Low.                        |
| [#6210](https://github.com/mantidproject/mantid/issues/6210) (Trac 5364) | Muon: bug calculating dead time                                                 | Changes to minimiser stop conditions look sensible.                                                            | Medium.                     |
| [#6237](https://github.com/mantidproject/mantid/issues/6237) (Trac 5391) | Invert does not work on MaskWorkspaceToCalFile                                  | Small change to allow the mask flag to be inverted, property was present before, just not implemented.         | Low.                        |
| [#6239](https://github.com/mantidproject/mantid/issues/6239) (Trac 5393) | SaveMask fails with WISH data                                                   | Changes look okay. Should stop throwing exception if zero detector ids found.                                  | Medium.                     |
| [#6240](https://github.com/mantidproject/mantid/issues/6240) (Trac 5394) | Instrument View: Save mask does not apply the mask first.                       | Very simple change made.                                                                                       | Low.                        |
| [#6242](https://github.com/mantidproject/mantid/issues/6242) (Trac 5396) | Instrument View: Improve the text under the save button.                        | Simple change to display text                                                                                  | Low.                        |
| [#5682](https://github.com/mantidproject/mantid/issues/5682) (Trac 4835) | Improve the integration of peaks using IkedaCarpeter fits of TOF data.          | High number of changes, but these are confined to the expected areas of change. Will require thorough testing. | Medium.                     |
| [#6051](https://github.com/mantidproject/mantid/issues/6051) (Trac 5205) | Exceptions in Load with mixed file types.                                       | These changes appear concise and sensible.                                                                     | Medium.                     |
| [#6145](https://github.com/mantidproject/mantid/issues/6145) (Trac 5299) | Instruments with detectors missing from the definition cannot be displayed      | Should handle indexes < 0 properly. Will require thorough testing.                                             | Low.                        |
| [#6147](https://github.com/mantidproject/mantid/issues/6147) (Trac 5301) | Update needed to offspec definition.                                            | Update to the instrument definition xml file.                                                                  | Low.                        |
| [#6156](https://github.com/mantidproject/mantid/issues/6156) (Trac 5310) | Python API switches leave some commands unchanged.                              | Small tested change.                                                                                           | Low.                        |
| [#6241](https://github.com/mantidproject/mantid/issues/6241) (Trac 5395) | Run object usage is different in the new API                                    | Well tested change with modifications made in the expected code.                                               | Low.                        |
| [#6002](https://github.com/mantidproject/mantid/issues/6002) (Trac 5156) | Update to offspec scripts.                                                      | Limited scope of changes. Changes look reasonable. Should not be dangerous.                                    | Low.                        |

### Are any changes included in the release that are not accounted for above?

### Is the collective effect of the changes acceptably low risk?

### Code Reviewer Recommendation

This patch contains some important fixes to the known issues listed above. We anticipate a noticeable improvement to stability as a result of these changes.

Most of the changes have been implemented without touching core areas of the Mantid functionality. Given the scope of most of these changes, we have a low probability of failure.
