=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------

ALC
###

- Choosing *run_start* or *run_end* as the log value will now work correctly - the values are taken as seconds relative to the start time of the first run.

Muon Analysis
#############

- When reusing the same plot window, an option has been added on the Settings tab to control how many previous fits are kept. It can be adjusted to 0 (remove all previous fits; default pre-Mantid 3.7), 1 (keep just one previous fit plus this one; new default) or higher.

- *run_start* and *run_end* are now available as log values to use in the results table. They can be inserted either as text (ISO-formatted date strings) or as seconds since the start of the first run.

- Documentation has been moved to the Mantid help system.

Algorithms
----------

- :ref:`MaxEnt <algm-MaxEnt>`: MaxEnt has a new property, *ComplexImage*, which can be set
  to *False* when the imaginary part of the image should not be taken into account for the
  calculations.

- :ref:`MaxEnt <algm-MaxEnt>`: The expression for the second derivative of real (pos/neg) images has been corrected.

- :ref:`MaxEnt <algm-MaxEnt>`: The reconstructed image is always a point data workspace. The reconstructed data is of the same type as the input workspace.

- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`: the *Frequency* parameter is now supplied in MHz rather than megaradians per second.

- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`: The first column of the output phase table now shows spectrum numbers rather than workspace indices.

- :ref:`PlotAsymmetryByLogValue <algm-PlotAsymmetryByLogValue>`: If *run_start* or *run_end* are chosen as the log to use, the values are taken as seconds relative to the start time of the first run.

- :ref:`LoadMuonNexus <algm-LoadMuonNexus>`: Fixed loading of certain v1 NeXus files converted from other formats that did not contain number of good frames.

- :ref:`LoadMuonNexus <algm-LoadMuonNexus>`: Now loads the correct detector IDs, whether the whole file is loaded or just a selection of spectra. Correctly handles muon v2 Nexus files, in which one spectrum can map to multiple detectors.

Fit Functions
-------------

- :ref:`Keren <func-Keren>`: A bug was fixed so that the field comes out in the correct units.

|

`Full list of changes <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Muon%22>`_
on GitHub.
