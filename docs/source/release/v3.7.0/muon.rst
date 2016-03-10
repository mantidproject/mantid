=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------

Muon ALC
########

- The default directory for the last run is now set to the same directory selected for the first run `#15524 <https://github.com/mantidproject/mantid/pull/15524>`_

Muon Analysis
#############

- "Load current data" now works for ARGUS `#15474 <https://github.com/mantidproject/mantid/pull/15474>`_
- A bug was fixed where the user's choice of which group (or pair of groups) to plot was changed unexpectedly when the grouping table was updated `#15504 <https://github.com/mantidproject/mantid/pull/15504>`_

Algorithms
----------

- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`: speed increased by using a sequential fit. The shared frequency
  is found as a first step by grouping the spectra and fitting the asymmetry, then this frequency is treated as fixed
  in a sequential fit of all spectra individually. The grouping can be provided by the user or read automatically from
  the instrument definition. `#15191 <https://github.com/mantidproject/mantid/pull/15191>`_
- :ref:`FFT <algm-FFT>`: can now be run on muon workspaces without the need to run Rebin first. This is done by setting the
  property AcceptXRoundingErrors to true, meaning the algorithm will accept workspaces whose bin widths differ
  slightly. Large deviations will still produce a warning message or, if very large, an error.
  `#15325 <https://github.com/mantidproject/mantid/pull/15325>`_
- :ref:`AsymmetryCalc <algm-AsymmetryCalc>`: a bug was fixed where the algorithm failed to run on input WorkspaceGroups. `#15404 <https://github.com/mantidproject/mantid/pull/15404>`_
- :ref:`MaxEnt <algm-MaxEnt>`: MaxEnt now handles positive images `#15494 <https://github.com/mantidproject/mantid/pull/15494>`_
- :ref:`MaxEnt <algm-MaxEnt>`: Some improvements/fixes were added (output label, X rounding errors, and ability to increase the
  number of points in the image and reconstructed data) `#15606 <https://github.com/mantidproject/mantid/pull/15606>`

Fit Functions
-------------

|

`Full list of changes <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Muon%22>`_
on GitHub.
