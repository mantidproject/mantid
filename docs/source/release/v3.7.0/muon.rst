=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------

Muon ALC
########

- The default directory for the last run is now set to the same directory selected for the first run 
- Fixed an occasional crash seen when "Auto" was selected 
- Several usability fixes were made to the interface: 
  - The "Function" box was renamed "Take log value at" and moved next to the log to which it applies
  - The integration start time is initialised to the first good data rather than the first time bin, and is not reset when a new first run is loaded
  - The choice of periods is no longer reset when a new first run is loaded
- Performance has been improved slightly for the "Auto" latest file finding functionality.

Muon Analysis
#############

- "Load current data" now works for ARGUS 
- A bug was fixed where the user's choice of which group (or pair of groups) to plot was changed unexpectedly when the grouping table was updated 
- The "run number" field of the results table has been improved:

  - It now shows the range of co-added runs, e.g. *15189-90*, rather than just the first one
  - It shows the period(s) analysed, for example *15189: 1* or *15189: 2* for one period, and *15189: 1+2-3* for a combination.
  - No period number is shown in the case of single-period data, or if the sum of all periods is used.
  - These changes can be combined, e.g. *15189-91: 1+2*

- The "Run Information" box on the Home tab has been corrected for co-added sets of runs. The information shown now applies to all runs, where previously some of it was relevant to the first only: 

  - *Runs* label deals with non-consecutive ranges
  - *Sample Temperature* and *Sample Magnetic Field* are a range if not all the same value
  - *Average Temperature* is calculated from all logs
  - *Start* and *End* are the earliest start and latest end

- When the window is resized, all widgets within the window should now resize with it. This enables the interface to be used on smaller screens. 
- "Plot/Remove guess" now deals correctly with the case when a new run is loaded. 
- When plotting data from a new run in the same window as the previous plot, previous fits now remain on the graph, to enable easy comparison between datasets. They can be removed with the "Clear fit curves" option. 
- A crash was fixed when loading data on Linux.
- When loading a new run with a different main field direction, the correct grouping for the new field direction is now always loaded. 

Algorithms
----------

- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`: speed increased by using a sequential fit. The shared frequency
  is found as a first step by grouping the spectra and fitting the asymmetry, then this frequency is treated as fixed
  in a sequential fit of all spectra individually. The grouping can be provided by the user or read automatically from
  the instrument definition. 
- :ref:`FFT <algm-FFT>`: can now be run on muon workspaces without the need to run Rebin first. This is done by setting the
  property AcceptXRoundingErrors to true, meaning the algorithm will accept workspaces whose bin widths differ
  slightly. Large deviations will still produce a warning message or, if very large, an error.
- :ref:`FFT <algm-FFT>`: added property *AutoShift* to enable automatic phase correction for workspaces not centred at zero.
- :ref:`AsymmetryCalc <algm-AsymmetryCalc>`: a bug was fixed where the algorithm failed to run on input WorkspaceGroups.
- :ref:`MaxEnt <algm-MaxEnt>`: MaxEnt now handles positive images 
- :ref:`MaxEnt <algm-MaxEnt>`: Some improvements/fixes were added (output label, X rounding errors and ability to increase the number of points in the image and reconstructed data) 
- :ref:`MaxEnt <algm-MaxEnt>`: *AutoShift* property was added. As in :ref:`FFT <algm-FFT>` this property allows for automatic phase correction for workspaces not centred at zero 
- :ref:`LoadMuonNexus <algm-LoadMuonNexus>`: If the NeXus file (version 1) does not contain a grouping entry, or the grouping entry it contains is invalid, then the grouping will be loaded from the IDF. This enables use of such files in the ALC interface.

Fit Functions
-------------

- :ref:`Keren <func-Keren>` has been added as a new fit function - Amit Keren's 
  generalisation of the Abragam relaxation function to a longitudinal field,
  for fitting the time-dependent muon polarisation.

|

`Full list of changes <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Muon%22>`_
on GitHub.
