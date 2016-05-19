========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- New CNCS formula to calculate T0 accounts for different phasing of the choppers since August 2015
- The documentation for all calibration approaches, including PSD tube calibration has been pulled together :ref:`here<Calibration>`

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

Crystal Field
-------------

A fitting function was added (:ref:`CrystalFieldSpectrum <func-CrystalFieldSpectrum>`) that fits crystal field parameters to a spectrum.
It is based on fortran program FOCUS which was translated into C++.

Phonon DOS
----------

The old PySlice routine to compute the phonon DOS from powder data using the incoherent approximation has
been ported and is now a Mantid Python Algorithm :ref:`ComputeIncoherentDOS <algm-ComputeIncoherentDOS`
