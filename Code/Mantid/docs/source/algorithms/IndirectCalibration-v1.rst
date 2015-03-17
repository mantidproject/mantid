.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a calibration workspace to be used with inelastic indirect reductions,
allowing for the correction of relative detector intensities.

Either a single run file or range of runs in *.raw* format can be given to the
algorithm which are then merged into a single run using :ref:`MergeRuns
<algm-MergeRuns>`, a flat background tis then calculated and normalised to give
the output workspace.

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

   Calibration workspace has 1 bin(s) and 51 spectra.

.. categories::
