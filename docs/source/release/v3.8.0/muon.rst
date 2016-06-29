=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------

Muon Analysis
#############

- When reusing the same plot window, an option has been added on the Settings tab to control how many previous fits are kept. It can be adjusted to 0 (remove all previous fits; default pre-Mantid 3.7), 1 (keep just one previous fit plus this one; new default) or higher.

Algorithms
----------

- :ref:`MaxEnt <algm-MaxEnt>`: MaxEnt has a new property, *ComplexImage*, which can be set
  to *False* when the imaginary part of the image should not be taken into account for the
  calculations.

- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`: the *Frequency* parameter is now supplied in MHz rather than megaradians per second.

- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`: The first column of the output phase table now shows spectrum numbers rather than workspace indices.

Fit Functions
-------------

- :ref:`Keren <func-Keren>`: A bug was fixed so that the field comes out in the correct units.

|

`Full list of changes <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Muon%22>`_
on GitHub.
