(v2.1.0-muon)=

# Muon Analysis

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Current Runs

The next and previous buttons have been changed to let the user cycle through the runs before the most up to date data. This means that the suer no longer has to select the files by finding them on the network.

The run number is also displayed in run information and is the title of the graph. This was changed after feedback from the users said it was too difficult to find out what run number was the current run.

## Loading appending files

The ability to load a range of runs which are summed together and displayed as one plot has been added. To use this feature you can select multiple files using the browse button or you can enter a load string in the following format:

- "C:/someLocation/MUSR0000001.nxs-C:/someLocation/MUSR0000009.nxs"
- "MUSR0000001-0000009"
- "MUSR0000001-9"
- "0000001-0000009"
- "0000001-9"

## Dead times

Two algorithms have been added. The first is {ref}`CalMuonDeadTime <algm-CalMuonDeadTime>` and the second is {ref}`ApplyDeadTimeCorr <algm-ApplyDeadTimeCorr>`. These have been integrated into Muon Analysis so that the user can apply the dead times directly from the nexus file before loading if they choose. They can also apply dead times from a table workspace that they have previously created with the CalMuonDeadTime.

## Fitting

On the data analysis tab the user is able to fit functions to data. The muon fit property browser is now a separate object which inherits from the main fit property browser. The categories displayed are full of only muon related functions. There is now a few more functions that have been added including GausDecay, GausOsc, and more.

## Settings

The option to hide the previous graphs has been added. Feedback from a user said it was difficult to see the graph they were working on if they had been working on others before. This kind of problem was especially noticeable on small screens where space for graphs is an issue.

## Notable bug fixes

- No longer crashes on Linux
- MuonRemoveExpDecay x values now get populated properly.
