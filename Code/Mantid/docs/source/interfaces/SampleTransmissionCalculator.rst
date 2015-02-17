Sample Transmission Calculator
==============================

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Sample Transmission Calculator
  :align: right
  :width: 350

The sample transmission calculator allows the calculation of the sample
transmission and scattering over a given wavelength range for a particular
sample material and geometry.

This UI is a front end for the :ref:`CalculateSampleTransmission
<algm-CalculateSampleTransmission>` algorithm.

The algorithm assumes a flat plate sample which is perpendicular to the beam.

Input Wavelength
~~~~~~~~~~~~~~~~

.. interface:: Sample Transmission Calculator
  :widget: gbRangeBinning
  :align: right
  :width: 350

The wavelength range to calculate transmission over can be given either by
providing a single binning range in the format of a start, width and end value or
by providing a bin parameter string as used in the :ref:`Rebin <algm-Rebin>`
algorithm.

Sample Details
~~~~~~~~~~~~~~

.. interface:: Sample Transmission Calculator
  :widget: gbSampleDetails
  :align: right
  :width: 350

The sample details required are the chemical formula which is to be given in the
format expected by the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
algorithm, number density in :math:`\mathrm{\AA{}}^3` and thickness in :math:`cm`.

Output
~~~~~~

.. interface:: Sample Transmission Calculator
  :widget: gbOutput
  :align: right
  :width: 350

The output is given in a plot of the transmission percentage over wavelength and
a table of statistics for the transmission (maximum, minimum, mean, median and
standard deviation) as well as the scattering percentage for all wavelengths.

.. categories:: Interfaces General
