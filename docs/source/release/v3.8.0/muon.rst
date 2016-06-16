=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------

Muon Analysis
#############

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
