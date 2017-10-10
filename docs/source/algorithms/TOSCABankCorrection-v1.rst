.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm attempts to automatically correct TOSCA data in which the
position of the sample has been moved and has affected the alignment of features
on the spectra from forward and backscattering detector banks.

The input workspace should be an energy transfer reduction, for the default
values of SearchRange and ClosePeakTolerance the X axis is assumed to be in
cm-1, however the X axis is not restricted to this unit.

The algorithm works by finding peaks of a given shape (using the :ref:`FindPeaks
<algm-FindPeaks>`) on both the forward and backscattering banks, either
selecting a peak in a given position or selecting the peak with the highest X
value and attempting to match them to what is believed to be the same feature on
the other bank.

A scale factor is then calculated for each bank that will align at least the
selected peak and in doing so will also align the majority of misaligned peaks
across the two banks.

The sacling factor is calculated as follows:

.. math::

  X_{centre} = \frac{X_{forward peak} + X_{back peak}}{2}

  SF_{forward} = \frac{X_{centre}}{X_{forward peak}}

  SF_{back} = \frac{X_{centre}}{X_{back peak}}

The corrected spectra are then rebinned to the input workspace (using
:ref:`RebinToWorkspace <algm-RebinToWorkspace>`) to preserve the X range and to
maintain bin alignment.

The sum spectra (containing both forward and back scattering detectors) is then
recalculated by averaging the intensities of the two corrected spectra, this
compensates for the broader peaks seen on the original sum spectra due to the
misalignment of the peaks.

.. note::
  This algorithm is only intended to provide an approximation of what the
  measured spectra would look like if the sample was in the expected sample
  position.

Workflow
--------

.. diagram:: TOSCABankCorrection-v1_wkflw.dot


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Automatic peak selection.**

.. testcode:: ExTOSCABankCorrectionAutomatic

    original_reduction = Load('TSC14007_graphite002_red.nxs')

    corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = \
      TOSCABankCorrection(InputWorkspace=original_reduction)

    print('Target peak centre: %.f' % peak_position)

Output:

.. testoutput:: ExTOSCABankCorrectionAutomatic

    Target peak centre: 1080

**Example - Manual peak selection.**

.. testcode:: ExTOSCABankCorrectionManual

    original_reduction = Load('TSC14007_graphite002_red.nxs')

    corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = \
      TOSCABankCorrection(InputWorkspace=original_reduction,
                          PeakPosition='715')

    print('Target peak centre: %.f' % peak_position)

Output:

.. testoutput:: ExTOSCABankCorrectionManual

    Target peak centre: 714

.. categories::
