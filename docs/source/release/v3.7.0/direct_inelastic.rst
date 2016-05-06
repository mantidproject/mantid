========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- New CNCS formula to calculate T0 accounts for different phasing of the choppers since August 2015

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

Crystal Field
-------------

FOCUS fortran program that fits crystal field parameters is being translated into Mantid (C++ and python).
The release notes on this work will go here.

Phonon DOS
----------

The old PySlice routine to compute the phonon DOS from powder data using the incoherent approximation has
been ported and is now a Mantid Python Algorithm :ref:`ComputeIncoherentDOS <algm-ComputeIncoherentDOS`
