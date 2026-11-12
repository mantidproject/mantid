(v2.6.0-direct_inelastic)=

# Direct Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

```{figure} ../../images/2_6_release/DGSInterface.webp
---
class: screenshot
width: 415px
align: right
---
The DGS Reduction interface featuring *Live* and *Send to cluster* buttons
```

## New Features

### For ISIS instruments

- The ISIS inelastic reduction inside Mantid has all reduction parameters explicitly specified and described in Instrument_Parameters.xml files. There are only two places now to specify reduction parameters and these are the Instrument_Parameters.xml file, used as the reduction defaults, and the input parameters for inelastic reduction script itself used to overwrite values changed for the particular experiment.

- Simplified interface to the inelastic reduction scripts does not allow you to provide undefined/(mistyped) parameters to the scripts and allows you to change/specify only the parameters, present in the Instrument_Parameters.xml file. The parameters in the Instrument_Parameters.xml files are taken from real experiments and verified against vanadium and recent experimental data. The Mantid system tests check reduction against the verified results of real experiments.

### DGS Reduction Interface

- The DGS interface can now be used to reduce live data on ADARA-enabled beamlines at the SNS (currently HYSPEC and SEQUOIA, see screenshot).

  - If a connection to the live stream is available, and your default instrument (under the MantidPlot preferences) matches the one selected in the interface, then the *Use live stream* button will appear.
  - Selecting this will fix and/or disable certain options, but others are available as normal.
  - Hitting *Reduce* will launch the live reduction, which will read data starting from the time at which *Reduce* is pressed and continue until cancelled or the run ends.
  - The output workspace will update every few seconds, as will any plots generated off of it.
  - Note also the Step Scan Analysis interface, which enables the live analysis of rocking curve runs.

- DgsReduction is now enabled for parallel reduction on FERMI at the SNS.

  - Submission is enabled only from the analysis.sns.gov machines
  - For multiple files processed separately, each file correspond to a separate job.
  - There are some instructions in ticket [#8297](https://github.com/mantidproject/mantid/issues/8297) (Trac 7451)
  - The status of the jobs can be monitored in the "Remote jobs" tab, as seen in the Release Notes

- {doc}`Load-like syntax </api/python/mantid/api/MultipleFileProperty>` is allowed for multiple file processing
