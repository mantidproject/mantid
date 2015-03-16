.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a calibration workspace to be used with inelastic indirect reductions,
allowing for the correction of relative detector intensities.

Usage
-----

**Example - create calibration workspace for IRIS**

.. include:: ../usagedata-note.txt

.. testcode:: ExIndirectCalibrationSimple

   # Create a calibration workspace
   cal_ws = IndirectCalibration(InputFiles='IRS26173.raw',
                                DetectorRange='3,53',
                                PeakRange='62500,65000',
                                BackgroundRange='59000,61500')

   print 'Calibration workspace has %d bin(s) and %d spectra.' % (
          cal_ws.blocksize(), cal_ws.getNumberHistograms())

Output:

.. testoutput:: ExIndirectCalibrationSimple

   Calibration workspace has 1 bin(s) and 50 spectra.

.. categories::
