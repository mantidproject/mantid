Sample Transmission Calculator
==============================

.. contents:: Table of Contents
  :local:

Overview
--------

.. figure:: /images/Sample_Transmission_Calculator_interface.png
  :align: right
  :width: 500


The sample transmission calculator allows the calculation of the sample
transmission and scattering over a given wavelength range for a particular
sample material and geometry.

This UI is a front end for the :ref:`CalculateSampleTransmission
<algm-CalculateSampleTransmission>` algorithm.

The algorithm assumes a flat plate sample which is perpendicular to the beam.

Input Wavelength
~~~~~~~~~~~~~~~~

The wavelength range to calculate transmission over can be given either by
providing a single binning range in the format of a start, width and end value or
by providing a bin parameter string as used in the :ref:`Rebin <algm-Rebin>`
algorithm.

Sample Details
~~~~~~~~~~~~~~

The sample details required are the chemical formula which is to be given in the
format expected by the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
algorithm, number density per atom in :math:`atoms/\mathrm{\AA{}}^3` and
thickness in :math:`cm`.

Output
~~~~~~

The output is given in a plot of the transmission percentage over wavelength and
a table of statistics for the transmission (maximum, minimum, mean, median and
standard deviation) as well as the scattering percentage for all wavelengths.

.. categories:: Interfaces General
